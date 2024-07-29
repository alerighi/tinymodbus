#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

/* include implementation of the library */
#define TMB_IMPLEMENTATION
#include <tinymodbus.h>

#define SERIAL_CONFIG_SEPARATOR ", "
#define TCP_PORT_SEPARATOR ": "

#define DBG(fmt, ...) fprintf(stderr, "debug: " fmt "\n", ##__VA_ARGS__)
#define ERR(fmt, ...) fprintf(stderr, "error: " fmt "\n", ##__VA_ARGS__)
#define DIE(fmt, ...)                                       \
    do {                                                    \
        fprintf(stderr, "fatal: " fmt "\n", ##__VA_ARGS__); \
        exit(1);                                            \
    } while (0)

// clang-format off
static const struct option long_options[] = {
    {"help", 0, NULL, 'h'},
    {"address", 1, NULL, 'a'},
    {"read", 1, NULL, 'r'},
    {"write", 1, NULL, 'w'},
    {"holding-register", 0, NULL, 'H'},
    {"coil", 0, NULL, 'c'},
    {"input-register", 0, NULL, 'i'},
    {"discrete-input", 0, NULL, 'd'},
    {"tcp", 1, NULL, 'T'},
    {"rtu", 1, NULL, 'R'},
    {"ascii", 1, NULL, 'A'},
    { NULL, 0, NULL, 0 },
};
static const char *short_options = "ha:r:w:HcidT:R:A:";
// clang-format on

static void usage(const char *progname) {
    fprintf(stderr, "Usage: %s [-a address] [-q quantity] [-v value] [-rwRc]\n", progname);
    fprintf(stderr, "  -h, --help                     show this help message\n");
    fprintf(stderr, "  -a, --address <address>        Modbus address for the operation\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Operation to perform:\n");
    fprintf(stderr, "  -r, --read <quantity>          perform a read of quantity registers (default: 1)\n");
    fprintf(stderr, "  -w, --write <value,[value...]> perform a write operation, with the specified value\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Type of resource to operate on:\n");
    fprintf(stderr, "  -H, --holding-register         operate on holding registers (default)\n");
    fprintf(stderr, "  -c, --coil                     operate on coils\n");
    fprintf(stderr, "  -d, --discrete-input           operate on discrete inputs\n");
    fprintf(stderr, "  -i, --input-register           operate on input registers\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Connection modes (specify one):\n");
    fprintf(stderr, "  -T, --tcp <host[:port]>        connect using TCP/IP\n");
    fprintf(stderr, "  -R, --rtu <serial>             connect using RTU\n");
    fprintf(stderr, "  -A, --ascii <serial>           connect using ASCII\n");
    fprintf(stderr, " where <serial> is: port[,baud[,bits[,parity[,stop]]]]\n");
    fprintf(stderr, " example: /dev/ttyUSB0,9600,8,N,1\n");
}

static tmb_error_t parse_serial_connection_string(char *string, tmb_posix_transport_serial_config_t *config) {
    TMB_ON_FALSE_RETURN(config != NULL || string != NULL, TMB_E_INVALID_ARGUMENTS);

    char *token = strtok(string, SERIAL_CONFIG_SEPARATOR);
    if (token == NULL) {
        return TMB_E_INVALID_ARGUMENTS;
    }

    config->device = token;

    /* parse baud */
    token = strtok(NULL, SERIAL_CONFIG_SEPARATOR);
    if (token == NULL) {
        return TMB_SUCCESS;
    }
    config->baudrate = strtoul(token, NULL, 10);

    /* parse data_bits */
    token = strtok(NULL, SERIAL_CONFIG_SEPARATOR);
    if (token == NULL) {
        return TMB_SUCCESS;
    }
    unsigned long value = strtoul(token, NULL, 10);
    switch (value) {
    case 7:
        config->data_bits = TMB_SERIAL_DATA_BITS_7;
        break;
    case 8:
        config->data_bits = TMB_SERIAL_DATA_BITS_8;
        break;
    default:
        return TMB_E_INVALID_ARGUMENTS;
    }

    /* parse parity */
    token = strtok(NULL, SERIAL_CONFIG_SEPARATOR);
    if (token == NULL) {
        return TMB_SUCCESS;
    }
    switch (token[0]) {
    case 'E':
    case 'e':
        config->parity = TMB_SERIAL_PARITY_EVEN;
        break;
    case 'O':
    case 'o':
        config->parity = TMB_SERIAL_PARITY_ODD;
        break;
    case 'N':
    case 'n':
        config->parity = TMB_SERIAL_PARITY_NONE;
        break;
    default:
        return TMB_E_INVALID_ARGUMENTS;
    }

    /* parse stop_bits */
    token = strtok(NULL, "");
    if (token == NULL) {
        return TMB_SUCCESS;
    }
    value = strtoul(token, NULL, 10);
    switch (value) {
    case 1:
        config->stop_bits = TMB_SERIAL_STOP_BITS_1;
        break;
    case 2:
        config->stop_bits = TMB_SERIAL_STOP_BITS_2;
        break;
    default:
        return TMB_E_INVALID_ARGUMENTS;
    }

    return TMB_SUCCESS;
}

static tmb_error_t parse_tcp_connection_string(char *string, tmb_posix_transport_tcp_config_t *config) {
    TMB_ON_FALSE_RETURN(config == NULL || string == NULL, TMB_E_INVALID_ARGUMENTS);

    config->host = strtok(string, TCP_PORT_SEPARATOR);
    if (config->host == NULL) {
        return TMB_E_INVALID_ARGUMENTS;
    }

    char *port_str = strtok(NULL, "");
    if (port_str != NULL) {
        unsigned long value = strtoul(port_str, NULL, 10);
        if (value > UINT16_MAX) {
            return TMB_E_INVALID_ARGUMENTS;
        }
        config->port = value;
    }

    return TMB_SUCCESS;
}

int main(int argc, char *argv[]) {
    uint16_t address = 1;
    uint16_t quantity = 1;
    uint16_t value = 0;
    int do_write = 0;
    int coil = 0;
    int discrete_input = 0;
    int input_register = 0;
    uint8_t slave_number = 1;
    tmb_posix_transport_config_t config = {
        .transport_protocol = TMB_TRANSPORT_PROTOCOL_RTU,
        .serial = TMB_POSIX_TRANSPORT_SERIAL_CONFIG_DEFAULT,
    };
    bool transport_configured = false;

    int opt;
    while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            exit(0);

        case 'T':
            if (transport_configured) {
                DIE("transport already configured");
            }
            transport_configured = true;
            config.transport_protocol = TMB_TRANSPORT_PROTOCOL_TCPIP;
            config.tcp = TMB_POSIX_TRANSPORT_TCP_CONFIG_DEFAULT;
            if (parse_tcp_connection_string(optarg, &config.tcp) != TMB_SUCCESS) {
                DIE("invalid TCP configuration");
            }

        case 'A':
            config.transport_protocol = TMB_TRANSPORT_PROTOCOL_ASCII;
            /* fall trough */

        case 'R':
            if (transport_configured) {
                DIE("transport already configured");
            }
            transport_configured = true;
            if (parse_serial_connection_string(optarg, &config.serial) != TMB_SUCCESS) {
                DIE("invalid serial configuration");
            }
            break;

        case 'a':
            address = strtoul(optarg, NULL, 10);
            break;

        case 'r':
            quantity = strtoul(optarg, NULL, 10);
            break;

        case 'c':
            coil = 1;
            break;

        case 'w':
            value = strtoul(optarg, NULL, 10);
            do_write = 1;
            break;

        case 'd':
            discrete_input = 1;
            break;

        case 'i':
            input_register = 1;
            break;

        case 'n':
            slave_number = strtoul(optarg, NULL, 10);
            break;

        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!transport_configured) {
        DIE("must specify at leas one of: --tcp, --rtu, --ascii");
    }

    DBG("creating transport");
    tmb_transport_t *transport;
    tmb_error_t error = tmb_posix_transport_new(&config, &transport);
    if (error != TMB_SUCCESS) {
        DIE("tmb_posix_transport_new(): %d", error);
    }

    DBG("init Modbus interface");
    uint8_t buffer[TMB_ADU_TCPIP_MAX_SIZE];
    tmb_handle_t handle;
    error = tmb_init(&handle, TMB_MODE_CLIENT, config.transport_protocol, buffer, sizeof(buffer), transport);
    if (error != TMB_SUCCESS) {
        DIE("tmb_init(): %d", error);
    }

    DBG("setting device address");
    error = tmb_client_set_device_address(&handle, slave_number);
    if (error != TMB_SUCCESS) {
        DIE("tmb_client_set_device_address(): %d", error);
    }

    if (do_write) {
    } else {
        /* read */
        if (coil) {
            DBG("read coils from %04X for %u", address, quantity);
            uint8_t values[255];
            error = tmb_read_coils(&handle, address, quantity, values);
            if (error != TMB_SUCCESS) {
                DIE("tmb_read_coils() error: %d", error);
            }

            for (size_t i = 0; i < quantity / 8 + (quantity % 8); i++) {
                printf("reg[%zu] = %02x\n", i, values[i]);
            }
        } else if (discrete_input) {
            DBG("read discrete inputs from %04X for %u", address, quantity);
            uint8_t values[255];
            error = tmb_read_discrete_inputs(&handle, address, quantity, values);
            if (error != TMB_SUCCESS) {
                DIE("tmb_read_discrete_inputs() error: %d", error);
            }

            for (size_t i = 0; i < quantity / 8 + (quantity % 8); i++) {
                printf("reg[%zu] = %02x", i, values[i]);
            }
        } else if (input_register) {
            DBG("read discrete inputs from %04X for %u", address, quantity);
            uint16_t values[255];
            error = tmb_read_input_registers(&handle, address, quantity, values);
            if (error != TMB_SUCCESS) {
                DIE("tmb_read_input_registers() error: %d", error);
            }
            for (size_t i = 0; i < quantity; i++) {
                printf("reg[%zu] = %u", i, values[i]);
            }
        } else {
            DBG("read holding registers from %04X for %u", address, quantity);

            uint16_t values[255];
            error = tmb_read_holding_registers(&handle, address, quantity, values);
            if (error != TMB_SUCCESS) {
                DIE("tmb_read_holding_registers() error: %d", error);
            }
            for (size_t i = 0; i < quantity; i++) {
                printf("reg[%zu] = %u\n", i, values[i]);
            }
        }
    }

    tmb_posix_transport_free(transport);
}
