#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

/* include implementation of the library */
#define TMB_IMPLEMENTATION
#include <tinymodbus.h>

// clang-format off
const struct option long_options[] = {
    {"address", 1, NULL, 'a'},
    {"quantity", 1, NULL, 'q'},
    {"value", 1, NULL, 'v'},
    {"read", 0, NULL, 'r'},
    {"write", 0, NULL, 'w'},
    {"register", 0, NULL, 'R'},
    {"coil", 0, NULL, 'c'},
    {"help", 0, NULL, 'h'},
    {"port", 1, NULL, 'p'},
    {"host", 1, NULL, 'H'},
    {"input-register", 0, NULL, 'i'},
    {"discrete-input", 0, NULL, 'd'},
    { NULL, 0, NULL, 0 },
};
// clang-format on

void usage(const char *progname) {
    fprintf(stderr, "Usage: %s [-a address] [-n quantity] [-v value] [-rwRc]\n", progname);
    fprintf(stderr, "  -h, --help                 show this help message\n");
    fprintf(stderr, "  -a, --address <address>    Modbus address for the operation\n");
    fprintf(stderr, "  -q, --quantity <quantity>  quantity of registers/coils to read/write\n");
    fprintf(stderr, "  -v, --value <value>        value to write\n");
    fprintf(stderr, "  -r, --read                 perform a read operation (default)\n");
    fprintf(stderr, "  -w, --write                perform a write operation\n");
    fprintf(stderr, "  -R, --registers            operate on registers (default)\n");
    fprintf(stderr, "  -c, --coils                operate on coils\n");
    fprintf(stderr, "  -d, --discrete-inputs      operate on discrete inputs\n");
    fprintf(stderr, "  -i, --input-register       operate on input registers\n");
    fprintf(stderr, "  -p, --port <port>          TCP port to connect to\n");
    fprintf(stderr, "  -H, --host <hostname>      hostname/IP to connect to\n");
}

int main(int argc, char *argv[]) {
    uint16_t address = 1;
    uint16_t quantity = 1;
    uint16_t value = 0;
    const char *host = "localhost";
    uint16_t port = 502;
    int do_write = 0;
    int coil = 0;
    int discrete_input = 0;
    int input_register = 0;

    int opt;
    while ((opt = getopt_long(argc, argv, "ha:q:v:rwRcp:H:id", long_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;

        case 'a':
            address = strtoul(optarg, NULL, 10);
            break;

        case 'q':
            quantity = strtoul(optarg, NULL, 10);
            break;

        case 'v':
            value = strtoul(optarg, NULL, 10);
            break;

        case 'p':
            value = strtoul(optarg, NULL, 10);
            break;

        case 'H':
            host = optarg;
            break;

        case 'c':
            coil = 1;
            break;

        case 'w':
            do_write = 1;
            break;

        case 'd':
            discrete_input = 1;
            break;

        case 'i':
            input_register = 1;
            break;

        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    tmb_transport_t *socket_transport = tmb_posix_transport_new_tcpip(host, port);
    if (socket_transport == NULL) {
        fprintf(stderr, "cannot initialize transport\n");
        return EXIT_FAILURE;
    }

    uint8_t buffer[TMB_ADU_TCPIP_MAX_SIZE];

    tmb_handle_t handle;
    tmb_error_t error =
            tmb_init(&handle, TMB_MODE_CLIENT, TMB_ENCAPSULATION_TCPIP, buffer, sizeof(buffer), socket_transport);
    if (error != TMB_SUCCESS) {
        fprintf(stderr, "tmb_init() error: %d", error);
        return EXIT_FAILURE;
    }

    if (do_write) {
    } else {
        /* read */
        if (coil) {
            uint8_t values[255];
            error = tmb_read_coils(&handle, address, quantity, values);
            if (error != TMB_SUCCESS) {
                fprintf(stderr, "tmb_read_coils() error: %d", error);
                return EXIT_FAILURE;
            }

            for (size_t i = 0; i < quantity / 8 + (quantity % 8); i++) {
                printf("reg[%zu] = %02x", i, values[i]);
            }
        } else if (discrete_input) {
            uint8_t values[255];
            error = tmb_read_discrete_inputs(&handle, address, quantity, values);
            if (error != TMB_SUCCESS) {
                fprintf(stderr, "tmb_read_discrete_inputs() error: %d", error);
                return EXIT_FAILURE;
            }

            for (size_t i = 0; i < quantity / 8 + (quantity % 8); i++) {
                printf("reg[%zu] = %02x", i, values[i]);
            }
        } else if (input_register) {
            uint16_t values[255];
            error = tmb_read_input_registers(&handle, address, quantity, values);
            if (error != TMB_SUCCESS) {
                fprintf(stderr, "tmb_read_input_registers() error: %d", error);
                return EXIT_FAILURE;
            }
            for (size_t i = 0; i < quantity; i++) {
                printf("reg[%zu] = %u", i, values[i]);
            }
        } else {
            uint16_t values[255];
            error = tmb_read_holding_registers(&handle, address, quantity, values);
            if (error != TMB_SUCCESS) {
                fprintf(stderr, "tmb_read_holding_registers() error: %d", error);
                return EXIT_FAILURE;
            }
            for (size_t i = 0; i < quantity; i++) {
                printf("reg[%zu] = %u", i, values[i]);
            }
        }
    }

    tmb_posix_transport_free(socket_transport);
}
