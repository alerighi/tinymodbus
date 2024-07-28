/**
 * \file tinymodbus.h
 * \brief Tiny Modbus - A small, header-only, portable and compliant
 *      Modbus implementation, suitable for embedded systems.
 * \copyright Copyright (c) 2024
 * \author Alessandro Righi <alessandro.righi@alerighi.it>
 */

#ifndef TINYMODBUS_H
#define TINYMODBUS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* constant definitions */

/**
 * Maximum number of addresses that the server can listen to.
 * This number can be increased or decreased. Consider that each
 * address takes memory!
 */
#define TMB_SERVER_MAX_ADDRESSES 10

/**
 * The Modbus broadcast address, as defined by the standard.
 */
#define TMB_ADDRESS_BROADCAST 0

/**
 * An address that signals that the Modbus server shall listen on
 * messages that arrives on any address.
 */
#define TMB_ADDRESS_ANY 256

/** Default port for Modbus TCP/IP */
#define TMB_DEFAULT_TCP_IP_PORT 502

#define TMB_PDU_MAX_SIZE 253
#define TMB_ADU_RTU_MAX_SIZE (1 + TMB_PDU_MAX_SIZE + 2)
#define TMB_ADU_ASCII_ADU_MAX_SIZE (1 + (1 + TMB_PDU_MAX_SIZE + 1) * 2 + 2)
#define TMB_ADU_TCPIP_MAX_SIZE (7 + TMB_PDU_MAX_SIZE)

/**
 * Protocol identifier for the Modbus protocol
 */
#define TMB_MODBUS_PROTOCOL_IDENTIFIER 0

#define TMB_READ_COIL_MIN_QUANTITY 1
#define TMB_READ_COIL_MAX_QUANTITY 2000

#define TMB_READ_DISCRETE_INPUT_MIN_QUANTITY 1
#define TMB_READ_DISCRETE_INPUT_MAX_QUANTITY 2000

#define TMB_READ_HOLDING_REGISTER_MIN_QUANTITY 1
#define TMB_READ_HOLDING_REGISTER_MAX_QUANTITY 125

#define TMB_READ_INPUT_REGISTER_MIN_QUANTITY 1
#define TMB_READ_INPUT_REGISTER_MAX_QUANTITY 125

#define TMB_WRITE_SINGLE_COIL_TRUE_VALUE 0xFF00
#define TMB_WRITE_SINGLE_COIL_FALSE_VALUE 0x0000

#define TMB_WRITE_MULTIPLE_COILS_MIN_QUANTITY 1
#define TMB_WRITE_MULTIPLE_COILS_MAX_QUANTITY 1968

#define TMB_WRITE_MULTIPLE_REGISTERS_MIN_QUANTITY 1
#define TMB_WRITE_MULTIPLE_REGISTERS_MAX_QUANTITY 123

/** Checks if a specified tmb_error_t is a Modbus Exception code */
#define TMB_ERROR_IS_MODBUS_EXCEPTION(error) (error > 0 && error < 256)

/** Returns true if the specified function code is of an error */
#define TMB_IS_FUNCTION_EXCEPTION_CODE(code) (code > 0x80)

/* public types */

/**
 * \typedef tmb_error_t
 * \brief The error type returned by all Tiny Modbus functions.
 *      Refer fo tmb_error enum for the various error codes.
 * \note For portability reasons this type does not refer to the enum
 */
typedef int tmb_error_t;

/**
 * \enum tmb_error
 * \brief Possible error codes of the Tiny Modbus library
 */
enum tmb_error {
    /** Operation completed successfully */
    TMB_SUCCESS = 0,

    /* the following errors are the one defined in the Modbus standard */

    /**
     * The function code received in the query is not
     * an allowable action for the server (or slave)
     */
    TMB_E_ILLEGAL_FUNCTION = 1,

    /**
     * The data address received in the query is not an
     * allowable address for the server (or slave)
     */
    TMB_E_ILLEGAL_DATA_ADDRESS = 2,

    /**
     * A value contained in the query data field
     * is not an allowable value for server (or slave)
     */
    TMB_E_ILLEGAL_DATA_VALUE = 3,

    /**
     * An unrecoverable error occurred while the server
     * (or slave) was attempting to perform the requested action.
     */
    TMB_E_SLAVE_DEVICE_FAILURE = 4,

    /**
     * The server (or slave) has accepted the request
     * and is processing it, but a long duration of time
     * will be required to do so.
     */
    TMB_E_ACKNOWLEDGE = 5,

    /**
     * The server (or slave) is engaged in processing a
     * long–duration program command.
     */
    TMB_E_SLAVE_DEVICE_BUSY = 6,

    /**
     * Specialized use in conjunction with function codes
     * 20 and 21 and reference type 6, to indicate that
     * the extended file area failed to pass a consistency check.
     */
    TMB_E_MEMORY_PARITY_ERROR = 8,

    /**
     * Specialized use in conjunction with gateways,
     * indicates that the gateway was unable to allocate
     * an internal communication path from the input
     * port to the output port for processing the request.
     */
    TMB_E_GATEWAY_PATH_UNAVAILABLE = 10,

    /**
     * Specialized use in conjunction with gateways,
     * indicates that no response was obtained from the
     * target device.
     */
    TMB_E_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND = 11,

    /* The following codes are used internally to the application */

    /** Operation failed without a specific error cause */
    TMB_FAILURE = 256,

    /**
     * Operation was ignored. Returning this codes from a callback
     * causes no response (neither success or failure) to be sent to
     * the client. Useful when listening on any address
     */
    TMB_IGNORED,

    /* The current operation timed out */
    TMB_E_TIMEOUT,

    /** An invalid argument has be provided to a function */
    TMB_E_INVALID_ARGUMENTS,

    /** The operation cannot be performed in the specified mode of operation
     * (i.e. requesting a client function when in server mode)
     */
    TMB_E_INVALID_MODE,

    /** The requested functionality is not implemented */
    TMB_E_NOT_IMPLEMENTED,

    /** There is no enough memory available to complete the operation */
    TMB_E_NO_MEMORY,

    /** There was an error sending/receiving from the transport */
    TMB_E_TRANSPORT,

};

/**
 * \typedef tmb_mode_t
 * \brief Mode of operation of the Modbus bus
 */
typedef enum {
    /** Operate in client mode */
    TMB_MODE_CLIENT,

    /** Operate in server mode */
    TMB_MODE_SERVER,
} tmb_mode_t;

/**
 * \typedef tmb_encapsulation_t
 * \brief Message encapsulation mode
 */
typedef enum {
    /** Encapsulation over serial port using binary data */
    TMB_ENCAPSULATION_RTU,

    /** Encapsulation over serial port using ASCII data */
    TMB_ENCAPSULATION_ASCII,

    /** Encapsulation to be used on TCP/IP sockets */
    TMB_ENCAPSULATION_TCPIP,
} tmb_encapsulation_t;

/**
 * \typedef tmb_transport_t
 * \brief Transport interface for the Modbus protocol
 */
typedef struct {
    /** User data pointer that is passed to the transport functions */
    void *user_data;

    /**
     * \brief Function to read bytes from the transport
     */
    int (*read)(void *user_data, uint8_t *buffer, size_t nbyte);

    /**
     * \brief Function to write bytes to the transport
     */
    int (*write)(void *user_data, const uint8_t *buffer, size_t nbyte);
} tmb_transport_t;

/**
 * \brief Collection of callbacks for the Modbus server
 */
typedef struct {
    void *user_data;

    tmb_error_t (*on_read_holding_register)(void *user_data, uint8_t address, uint16_t reg, uint16_t *value);
    tmb_error_t (*on_write_holding_register)(void *user_data, uint8_t address, uint16_t reg, uint16_t value);
} tmb_callbacks_t;

/* private types */

/**
 * \typedef tmb_function_t
 * \brief Modbus function code definition
 * \note Refer to the Modbus protocol specification for their explanation
 */
typedef enum {
    TMB_FUNCTION_READ_COILS = 1,
    TMB_FUNCTION_READ_DISCRETE_INPUTS = 2,
    TMB_FUNCTION_READ_HOLDING_REGISTERS = 3,
    TMB_FUNCTION_READ_INPUT_REGISTERS = 4,
    TMB_FUNCTION_WRITE_SINGLE_COIL = 5,
    TMB_FUNCTION_WRITE_SINGLE_REGISTER = 6,
    TMB_FUNCTION_READ_EXCEPTION_STATUS = 7,
    TMB_FUNCTION_DIAGNOSTIC = 8,
    TMB_FUNCTION_GET_COM_EVENT_COUNTER = 11,
    TMB_FUNCTION_GET_COM_EVENT_LOG = 12,
    TMB_FUNCTION_REPORT_SLAVE_ID = 17,
    TMB_FUNCTION_WRITE_MULTIPLE_COILS = 15,
    TMB_FUNCTION_WRITE_MULTIPLE_REGISTERS = 16,
    TMB_FUNCTION_READ_FILE_RECORD = 20,
    TMB_FUNCTION_WRITE_FILE_RECORD = 21,
    TMB_FUNCTION_MASK_WRITE_REGISTER = 22,
    TMB_FUNCTION_READ_WRITE_MULTIPLE_REGISTERS = 23,
    TMB_FUNCTION_READ_FIFO_QUEUE = 24,
    TMB_FUNCTION_ENCAPSULATED_TRANSPORT = 43,
} tmb_function_t;

/**
 * \typedef tmb_req_pdu_t
 * \brief A Modbus request PDU, as defined by the standard
 */
typedef struct {
    /* Modbus function code */
    uint8_t function_code;

    union {
        struct {
            /** Address to start read from */
            uint16_t start_address;

            /** Num of coils to read. Must be between 1 and 2000 */
            uint16_t quantity;
        } read_coils;

        struct {
            /** Address to start read from */
            uint16_t start_address;

            /** Num of inputs to read. Must be between 1 and 2000 */
            uint16_t quantity;
        } read_discrete_inputs;

        struct {
            /** Address to start read from */
            uint16_t start_address;

            /** Number of registers to read. Must be between 1 and 125 */
            uint16_t quantity;
        } read_holding_registers;

        struct {
            /** Address to start read from */
            uint16_t start_address;

            /** Number of registers to read. Must be between 1 and 125 */
            uint16_t quantity;
        } read_input_registers;

        struct {
            /** Address of the coil to write */
            uint16_t address;

            /** Value of the coil to write (either 0xFF00 or 0x0000) */
            uint16_t value;
        } write_single_coil;

        struct {
            /** Address of the register to write */
            uint16_t address;

            /** Register value */
            uint16_t value;
        } write_single_register;

        struct {
            /** Address to start write from */
            uint16_t start_address;

            /** Number of coils to write. Must be between 1 and 1968 */
            uint16_t quantity;

            /** Number of bytes in the payload */
            uint8_t byte_count;

            /** Bytes to write */
            const uint8_t *values;
        } write_multiple_coils;

        struct {
            /** Address to start write from */
            uint16_t start_address;

            /** Number of coils to write. Must be between 1 and 1968 */
            uint16_t quantity;

            /** Number of bytes to write */
            uint8_t byte_count;

            /** Register values */
            const uint16_t *values;
        } write_multiple_registers;
    };
} tmb_request_pdu_t;

/**
 * \typedef tmb_handle_t
 * \brief An handle to the Tiny Modbus instance. This is here only
 *      to allow for static allocation of the handle. It must be
 *      treated as a black-box.
 */
typedef struct {
    /** true if the current handle is valid (initialized) */
    bool is_valid;

    /** Buffer used to serialize requests/responses */
    uint8_t *buffer;

    /** Size of the buffer */
    size_t buffer_size;

    /** Current mode of operation */
    tmb_mode_t mode;

    /** Current encapsulation */
    tmb_encapsulation_t encapsulation;

    /** Current transport */
    const tmb_transport_t *transport;

    union {
        /** client-specific state */
        struct {
            uint8_t device_address;

            tmb_request_pdu_t request;

            uint16_t last_transaction_identifier;
        } client;

        /** server-specific state */
        struct {
            struct {
                uint16_t address;
                const tmb_callbacks_t *callbacks;
            } callbacks[TMB_SERVER_MAX_ADDRESSES];
        } server;
    };

} tmb_handle_t;

typedef struct {
    /* Modbus function code */
    uint8_t function_code;

    union {
        struct {
            /** Number of bytes in the response */
            uint8_t byte_count;

            /** Array of coil status */
            const uint8_t *coil_status;
        } read_coils;

        struct {
            /** Number of bytes in the response */
            uint8_t byte_count;

            /** Array of coil status */
            const uint8_t *input_status;
        } read_discrete_inputs;

        struct {
            /** Number of bytes in the response */
            uint8_t byte_count;

            /** Register values */
            const uint16_t *register_values;
        } read_holding_registers;

        struct {
            /** Number of bytes in the response */
            uint8_t byte_count;

            /** Register values */
            const uint16_t *register_values;
        } read_input_registers;

        struct {
            /** Address of the coil to write */
            uint16_t address;

            /** Value of the coil to write (either 0xFF00 or 0x0000) */
            uint16_t value;
        } write_single_coil;

        struct {
            /** Address of the register to write */
            uint16_t address;

            /** Register value */
            uint16_t value;
        } write_single_register;

        struct {
            /** Address to start write from */
            uint16_t start_address;

            /** Number of coils to write. Must be between 1 and 1968 */
            uint16_t quantity;
        } write_multiple_coils;

        struct {
            /** Address to start write from */
            uint16_t start_address;

            /** Number of coils to write. Must be between 1 and 1968 */
            uint16_t quantity;
        } write_multiple_registers;
    };
} tmb_response_pdu_t;

/**
 * Exception Modbus PDU, as defined by the standard
 */
typedef struct {
    /** The exception function code, that is function_code + 0x80 */
    uint8_t exception_function_code;

    /** The exception (error) code */
    uint8_t exception_code;
} tmb_exception_pdu_t;

/**
 * Type of the Modbus message
 */
typedef enum {
    /** A Modbus request */
    TMB_MESSAGE_TYPE_REQUEST,

    /** A Modbus response */
    TMB_MESSAGE_TYPE_RESPONSE,

    /** A Modbus response indicating an error */
    TMB_MESSAGE_TYPE_EXCEPTION,
} tmb_message_type_t;

/**
 * A generic decoded Modbus message
 */
typedef struct {
    tmb_message_type_t type;
    union {
        tmb_request_pdu_t request;
        tmb_response_pdu_t response;
        tmb_exception_pdu_t exception;
    };
} tmb_message_t;

/* public methods */

/**
 * \brief Initializes the specified Tiny Modbus handle
 * \param handle the handle to initialize
 * \param mode the mode of operation (client or server)
 * \param encapsulation the encapsulation to be used
 * \param transport the transport to be used. The transport struct shall live for the
 *      whole duration of the handle
 * \returns TMB_SUCCESS if initialized successfully, otherwise TMB_INVALID_ARG
 */
tmb_error_t tmb_init(tmb_handle_t *handle, tmb_mode_t mode, tmb_encapsulation_t encapsulation, uint8_t *buffer,
                     size_t buffer_size, const tmb_transport_t *transport);

/**
 * \brief Sets the slave address ID
 * \param handle handle to the Modbus instance
 * \param address address of the slave
 * \returns TMB_SUCCESS or TMB_INVALID_ARGUMENTS
 */
tmb_error_t tmb_client_set_device_address(tmb_handle_t *handle, uint8_t address);

/**
 * \brief Validate the given request object.
 * \param request the request to validate
 * \returns TMB_SUCCESS if the request is valid, otherwise an appropriate error code
 */
tmb_error_t tmb_request_validate(const tmb_request_pdu_t *request);

tmb_error_t tmb_client_send_request(tmb_handle_t *handle, const tmb_request_pdu_t *request,
                                    tmb_response_pdu_t *response);

tmb_error_t tmb_read_coils(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity, uint8_t *values);

tmb_error_t tmb_read_discrete_inputs(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity, uint8_t *values);

tmb_error_t tmb_read_holding_registers(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity,
                                       uint16_t *values);

tmb_error_t tmb_read_input_registers(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity, uint16_t *values);

tmb_error_t tmb_write_single_coil(tmb_handle_t *handle, uint16_t address, uint16_t value);

tmb_error_t tmb_write_single_register(tmb_handle_t *handle, uint16_t address, uint16_t value);

tmb_error_t tmb_write_multiple_coils(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity,
                                     const uint8_t *values);

tmb_error_t tmb_write_multiple_registers(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity,
                                         const uint16_t *values);

/**
 * \brief Sets the callback to use for server communication
 * \param handle The Modbus handle to be used
 * \param address The address that this callbacks respond to. A Modbus
 *      server can respond to more than one address, in this case call
 *      this function multiple times. Use TMB_ADDRESS_ANY for calling the
 *      callbacks for messages that arrive on any address.
 * \param callbacks The callback object. If NULL, removes the association
 *      with the specified server. They must be allocated to a memory buffer
 *      that shall live trough the whole time these callbacks are needed.
 */
tmb_error_t tmb_server_set_callback(tmb_handle_t *handle, uint16_t address, const tmb_callbacks_t *callbacks);

/**
 * \brief Run a Modbus server iteration, processing one message
 * \param handle the handle to the Modbus server
 * \returns TMB_SUCCESS if the request completed successfully, otherwise an error code
 */
tmb_error_t tmb_server_run_iteration(tmb_handle_t *handle);

/**
 * \brief Run the Modbus server forever, serving one request after another
 * \param handle the handle to the Modbus server
 * \return a failure code in case the server is unable to run. If no error,
 *      this function runs forever thus TMB_SUCCESS is never returned.
 */
tmb_error_t tmb_server_run_forever(tmb_handle_t *handle);

/**
 * \brief Returns a string representation of the provided error code
 * \param error the error code to convert
 * \returns a pointer to a static memory buffer containing the string
 *      representation of the error
 */
const char *tmb_strerror(tmb_error_t error);

/* optional functions for POSIX OS */

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

#define TMB_POSIX_SUPPORTED

/**
 * \brief Initializes a new TCP/IP transport using POSIX sockets
 * \param hostname hostname or IP address of the server
 * \param port port of the server
 * \returns a transport instance of successful connect, otherwise NULL
 */
tmb_transport_t *tmb_posix_transport_new_tcpip(const char *hostname, uint16_t port);

typedef enum {
    TMB_SERIAL_PARITY_NONE,
    TMB_SERIAL_PARITY_EVEN,
    TMB_SERIAL_PARITY_ODD,
} tmb_serial_parity_t;

/**
 * \brief Initializes a new TCP/IP transport using POSIX sockets
 * \param device device name to use (e.g. /dev/ttyXXXX)
 * \param baudrate the baudrate to use for the device (e.g. 9600)
 * \param stop_bits the number of stop bits to use (typically 1)
 * \param parity the parity to be used (typically TMB_SERIAL_PARITY_NONE)
 * \returns a tmb_transport_t instance in case of successful open, otherwise NULL
 */
tmb_transport_t *tmb_posix_transport_new_serial(const char *device, uint16_t baudrate, uint8_t stop_bits,
                                                tmb_serial_parity_t parity);

/**
 * \brief Closes a POSIX transport and frees all the allocated resources
 */
void tmb_posix_transport_free(tmb_transport_t *transport);

#endif

/* In only one C file, define this macro to include the implementation code */
#ifdef TMB_IMPLEMENTATION

#include <string.h>

/* private macro definitions */
#define TMB_ON_FALSE_RETURN(check, error) \
    do {                                  \
        if (!(check)) {                   \
            return (error);               \
        }                                 \
    } while (false)

#define TMB_ERROR_CHECK(expression)         \
    do {                                    \
        tmb_error_t __error = (expression); \
        if (__error != TMB_SUCCESS) {       \
            return __error;                 \
        }                                   \
    } while (false)

static const uint8_t TMB_ADU_ASCII_START_BYTE[] = { ':' };
static const uint8_t TMB_ADU_ASCII_END_BYTES[] = { '\r', '\n' };

#define TMB_ADU_TCPIP_SIZE_OFFSET 4
#define TMB_TO_HEX(i) ((i) <= 9 ? '0' + (i) : 'A' - 10 + (i))
#define TMB_UINT16(buffer, i) ((buffer[i] << 8) | buffer[i + 1])
#define TMB_RESPONSE_LOOKAHEAD_BYTES 2

/* private types */
typedef struct {
    tmb_encapsulation_t encapsulation;
    size_t capacity;
    size_t size;
    uint8_t *buffer;
} tmb_adu_t;

/* private functions */
static uint16_t tmb_crc16(const uint8_t *buffer, size_t buffer_size) {
    /* TODO */

    return 0;
}

static uint8_t tmb_lrc(const uint8_t *buffer, size_t buffer_size) {
    uint8_t lrc = 0;
    for (size_t i = 0; i < buffer_size; i++) {
        lrc += buffer[i];
    }

    return -lrc;
}

static tmb_error_t tmb_adu_add_uint8(tmb_adu_t *adu, uint8_t data) {
    switch (adu->encapsulation) {
    case TMB_ENCAPSULATION_RTU:
    case TMB_ENCAPSULATION_TCPIP:
        TMB_ON_FALSE_RETURN(adu->size < adu->capacity, TMB_E_NO_MEMORY);

        adu->buffer[adu->size++] = data;
        break;
    case TMB_ENCAPSULATION_ASCII:
        TMB_ON_FALSE_RETURN(adu->size + 1 < adu->capacity, TMB_E_NO_MEMORY);
        adu->buffer[adu->size++] = TMB_TO_HEX((data & 0xF0) >> 4);
        adu->buffer[adu->size++] = TMB_TO_HEX(data & 0x0F);
        break;
    }

    return TMB_SUCCESS;
}

static tmb_error_t tmb_adu_add_uint16(tmb_adu_t *adu, uint8_t data) {
    TMB_ERROR_CHECK(tmb_adu_add_uint8(adu, (data >> 8) & 0xFF));
    TMB_ERROR_CHECK(tmb_adu_add_uint8(adu, data & 0xFF));

    return TMB_SUCCESS;
}

static tmb_error_t tmb_adu_add_bytes(tmb_adu_t *adu, const uint8_t *bytes, size_t bytes_size) {
    TMB_ERROR_CHECK(adu->capacity - adu->size >= bytes_size);

    memcpy(&adu->buffer[adu->size], bytes, bytes_size);
    adu->size += bytes_size;

    return TMB_SUCCESS;
}

static tmb_error_t tmb_adu_init(tmb_adu_t *adu, uint8_t *buffer, size_t buffer_size, tmb_encapsulation_t encapsulation,
                                uint16_t transaction_identifier, uint8_t device_address) {
    memset(adu, 0, sizeof(tmb_adu_t));

    adu->encapsulation = encapsulation;
    adu->capacity = buffer_size;
    adu->buffer = buffer;

    switch (encapsulation) {
    case TMB_ENCAPSULATION_ASCII:
        /* add ascii start byte */
        TMB_ERROR_CHECK(tmb_adu_add_bytes(adu, TMB_ADU_ASCII_START_BYTE, sizeof(TMB_ADU_ASCII_START_BYTE)));

        /* fall trough */
    case TMB_ENCAPSULATION_RTU:
        TMB_ERROR_CHECK(tmb_adu_add_uint8(adu, device_address));
        break;

    case TMB_ENCAPSULATION_TCPIP:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, transaction_identifier));
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, TMB_MODBUS_PROTOCOL_IDENTIFIER));

        /* placeholder for real size, that will be populated later, when known */
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, 0));

        TMB_ERROR_CHECK(tmb_adu_add_uint8(adu, device_address));
        break;
    }

    return TMB_SUCCESS;
}

static tmb_error_t tmb_adu_finalize(tmb_adu_t *adu) {
    switch (adu->encapsulation) {
    case TMB_ENCAPSULATION_TCPIP: {
        uint16_t size = adu->size;

        adu->buffer[TMB_ADU_TCPIP_SIZE_OFFSET] = (size >> 8) * 0xff;
        adu->buffer[TMB_ADU_TCPIP_SIZE_OFFSET + 1] = size & 0xff;
        break;
    }

    case TMB_ENCAPSULATION_RTU:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, tmb_crc16(adu->buffer, adu->size)));
        break;

    case TMB_ENCAPSULATION_ASCII:
        TMB_ERROR_CHECK(tmb_adu_add_uint8(adu, tmb_lrc(adu->buffer, adu->size)));
        TMB_ERROR_CHECK(tmb_adu_add_bytes(adu, TMB_ADU_ASCII_END_BYTES, sizeof(TMB_ADU_ASCII_END_BYTES)));
        break;
    }

    return TMB_SUCCESS;
}

static tmb_error_t tmb_adu_serialize_request(tmb_adu_t *adu, const tmb_request_pdu_t *request) {
    switch (request->function_code) {
    case TMB_FUNCTION_READ_COILS:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->read_coils.start_address));
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->read_coils.quantity));
        break;
    case TMB_FUNCTION_READ_DISCRETE_INPUTS:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->read_discrete_inputs.start_address));
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->read_discrete_inputs.quantity));
        break;

    case TMB_FUNCTION_READ_HOLDING_REGISTERS:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->read_holding_registers.start_address));
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->read_holding_registers.quantity));
        break;
    case TMB_FUNCTION_READ_INPUT_REGISTERS:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->read_input_registers.start_address));
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->read_input_registers.quantity));
        break;
    case TMB_FUNCTION_WRITE_SINGLE_COIL:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->write_single_coil.address));
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->write_single_coil.value));
        break;
    case TMB_FUNCTION_WRITE_SINGLE_REGISTER:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->write_single_register.address));
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->write_single_register.value));
        break;
    case TMB_FUNCTION_WRITE_MULTIPLE_COILS:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->write_multiple_coils.start_address));
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->write_multiple_coils.quantity));
        TMB_ERROR_CHECK(tmb_adu_add_uint8(adu, request->write_multiple_coils.byte_count));
        TMB_ERROR_CHECK(tmb_adu_add_bytes(adu, request->write_multiple_coils.values,
                                          request->write_multiple_registers.byte_count));
        break;
    case TMB_FUNCTION_WRITE_MULTIPLE_REGISTERS:
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->write_multiple_registers.start_address));
        TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->write_multiple_registers.quantity));
        TMB_ERROR_CHECK(tmb_adu_add_uint8(adu, request->write_multiple_registers.byte_count));
        for (uint16_t i = 0; i < request->write_multiple_registers.quantity; i++) {
            TMB_ERROR_CHECK(tmb_adu_add_uint16(adu, request->write_multiple_registers.values[i]));
        }
        break;

    /* codes not implemented */
    case TMB_FUNCTION_READ_EXCEPTION_STATUS:
    case TMB_FUNCTION_DIAGNOSTIC:
    case TMB_FUNCTION_GET_COM_EVENT_COUNTER:
    case TMB_FUNCTION_GET_COM_EVENT_LOG:
    case TMB_FUNCTION_REPORT_SLAVE_ID:
    case TMB_FUNCTION_READ_FILE_RECORD:
    case TMB_FUNCTION_WRITE_FILE_RECORD:
    case TMB_FUNCTION_MASK_WRITE_REGISTER:
    case TMB_FUNCTION_READ_WRITE_MULTIPLE_REGISTERS:
    case TMB_FUNCTION_READ_FIFO_QUEUE:
    case TMB_FUNCTION_ENCAPSULATED_TRANSPORT:
    default:
        return TMB_E_ILLEGAL_FUNCTION;
    }

    return TMB_SUCCESS;
}

static size_t tmb_get_response_size(uint8_t function_code, uint8_t first_byte) {
    if (TMB_IS_FUNCTION_EXCEPTION_CODE(function_code)) {
        /** Exception PDU has a size of 2 */
        return 2;
    }

    switch (function_code) {
    case TMB_FUNCTION_READ_COILS:
        return 2 + first_byte;

    case TMB_FUNCTION_READ_DISCRETE_INPUTS:
        return 2 + first_byte;

    case TMB_FUNCTION_READ_HOLDING_REGISTERS:
        return 2 + first_byte;

    case TMB_FUNCTION_READ_INPUT_REGISTERS:
        return 2 + first_byte;

    case TMB_FUNCTION_WRITE_SINGLE_COIL:
        return 5;

    case TMB_FUNCTION_WRITE_SINGLE_REGISTER:
        return 5;

    case TMB_FUNCTION_WRITE_MULTIPLE_COILS:
        return 5;

    case TMB_FUNCTION_WRITE_MULTIPLE_REGISTERS:
        return 5;

    /* codes not implemented */
    case TMB_FUNCTION_READ_EXCEPTION_STATUS:
    case TMB_FUNCTION_DIAGNOSTIC:
    case TMB_FUNCTION_GET_COM_EVENT_COUNTER:
    case TMB_FUNCTION_GET_COM_EVENT_LOG:
    case TMB_FUNCTION_REPORT_SLAVE_ID:
    case TMB_FUNCTION_READ_FILE_RECORD:
    case TMB_FUNCTION_WRITE_FILE_RECORD:
    case TMB_FUNCTION_MASK_WRITE_REGISTER:
    case TMB_FUNCTION_READ_WRITE_MULTIPLE_REGISTERS:
    case TMB_FUNCTION_READ_FIFO_QUEUE:
    case TMB_FUNCTION_ENCAPSULATED_TRANSPORT:
    default:
        return TMB_E_ILLEGAL_FUNCTION;
    }

    return TMB_SUCCESS;
}

static tmb_error_t tmb_response_parse(tmb_response_pdu_t *response, uint8_t *buffer, size_t buffer_size) {
    TMB_ON_FALSE_RETURN(buffer == NULL || buffer_size < 2, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(tmb_get_response_size(buffer[0], buffer[1]) <= buffer_size, TMB_E_INVALID_ARGUMENTS);

    response->function_code = buffer[0];
    TMB_ON_FALSE_RETURN(TMB_IS_FUNCTION_EXCEPTION_CODE(response->function_code), TMB_E_INVALID_ARGUMENTS);

    switch (response->function_code) {
    case TMB_FUNCTION_READ_COILS:
        response->read_coils.byte_count = buffer[1];
        response->read_coils.coil_status = &buffer[2];
        break;

    case TMB_FUNCTION_READ_DISCRETE_INPUTS:
        response->read_discrete_inputs.byte_count = buffer[1];
        response->read_discrete_inputs.input_status = &buffer[2];
        break;

    case TMB_FUNCTION_READ_HOLDING_REGISTERS:
        response->read_holding_registers.byte_count = buffer[1];
        response->read_holding_registers.register_values = (uint16_t *)&buffer[2];
        break;

    case TMB_FUNCTION_READ_INPUT_REGISTERS:
        response->read_input_registers.byte_count = buffer[1];
        response->read_input_registers.register_values = (uint16_t *)&buffer[2];
        break;

    case TMB_FUNCTION_WRITE_SINGLE_COIL:
        response->write_single_coil.address = TMB_UINT16(buffer, 1);
        response->write_single_coil.value = TMB_UINT16(buffer, 3);
        break;

    case TMB_FUNCTION_WRITE_SINGLE_REGISTER:
        response->write_single_register.address = TMB_UINT16(buffer, 1);
        response->write_single_register.value = TMB_UINT16(buffer, 3);
        break;

    case TMB_FUNCTION_WRITE_MULTIPLE_COILS:
        response->write_multiple_coils.start_address = TMB_UINT16(buffer, 1);
        response->write_multiple_coils.quantity = TMB_UINT16(buffer, 3);
        break;

    case TMB_FUNCTION_WRITE_MULTIPLE_REGISTERS:
        response->write_multiple_registers.start_address = TMB_UINT16(buffer, 1);
        response->write_multiple_registers.quantity = TMB_UINT16(buffer, 3);
        break;

    /* codes not implemented */
    case TMB_FUNCTION_READ_EXCEPTION_STATUS:
    case TMB_FUNCTION_DIAGNOSTIC:
    case TMB_FUNCTION_GET_COM_EVENT_COUNTER:
    case TMB_FUNCTION_GET_COM_EVENT_LOG:
    case TMB_FUNCTION_REPORT_SLAVE_ID:
    case TMB_FUNCTION_READ_FILE_RECORD:
    case TMB_FUNCTION_WRITE_FILE_RECORD:
    case TMB_FUNCTION_MASK_WRITE_REGISTER:
    case TMB_FUNCTION_READ_WRITE_MULTIPLE_REGISTERS:
    case TMB_FUNCTION_READ_FIFO_QUEUE:
    case TMB_FUNCTION_ENCAPSULATED_TRANSPORT:
    default:
        return TMB_E_ILLEGAL_FUNCTION;
    }

    return TMB_SUCCESS;
}

static tmb_error_t tmb_send(tmb_handle_t *handle, const uint8_t *buffer, size_t buffer_size) {
    size_t transmitted_bytes = 0;
    while (transmitted_bytes < buffer_size) {
        /* write may send less bytes than requested. In this case, repeat the operation */
        int nbytes = handle->transport->write(handle->transport->user_data, buffer + transmitted_bytes,
                                              buffer_size - transmitted_bytes);
        if (nbytes <= 0) {
            return TMB_E_TRANSPORT;
        }
        transmitted_bytes += nbytes;
    }

    return TMB_SUCCESS;
}

static tmb_error_t tmb_receive(tmb_handle_t *handle, uint8_t *buffer, size_t buffer_size) {
    size_t received_bytes = 0;
    while (received_bytes < buffer_size) {
        /* write may send less bytes than requested. In this case, repeat the operation */
        int nbytes = handle->transport->read(handle->transport->user_data, buffer + received_bytes,
                                             buffer_size - received_bytes);
        if (nbytes <= 0) {
            return TMB_E_TRANSPORT;
        }
        received_bytes += nbytes;
    }

    return TMB_SUCCESS;
}

/* public functions implementation */

tmb_error_t tmb_init(tmb_handle_t *handle, tmb_mode_t mode, tmb_encapsulation_t encapsulation, uint8_t *buffer,
                     size_t buffer_size, const tmb_transport_t *transport) {
    TMB_ON_FALSE_RETURN(handle != NULL, TMB_E_INVALID_ARGUMENTS);
    memset(handle, 0, sizeof(tmb_handle_t));

    TMB_ON_FALSE_RETURN(buffer != NULL, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(transport != NULL, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(transport->read != NULL, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(transport->write != NULL, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(encapsulation != TMB_ENCAPSULATION_ASCII, TMB_E_NOT_IMPLEMENTED);

    handle->buffer = buffer;
    handle->buffer_size = buffer_size;
    handle->mode = mode;
    handle->encapsulation = encapsulation;
    handle->transport = transport;
    handle->is_valid = true;

    return TMB_SUCCESS;
}

tmb_error_t tmb_request_validate(const tmb_request_pdu_t *request) {
    TMB_ON_FALSE_RETURN(request != NULL, TMB_E_INVALID_ARGUMENTS);

    switch (request->function_code) {
    case TMB_FUNCTION_READ_COILS:
        TMB_ON_FALSE_RETURN(TMB_READ_COIL_MIN_QUANTITY <= request->read_coils.quantity &&
                                    request->read_coils.quantity <= TMB_READ_COIL_MAX_QUANTITY,
                            TMB_E_ILLEGAL_DATA_VALUE);
        break;
    case TMB_FUNCTION_READ_DISCRETE_INPUTS:
        TMB_ON_FALSE_RETURN(TMB_READ_DISCRETE_INPUT_MIN_QUANTITY <= request->read_discrete_inputs.quantity &&
                                    request->read_discrete_inputs.quantity <= TMB_READ_DISCRETE_INPUT_MAX_QUANTITY,
                            TMB_E_ILLEGAL_DATA_VALUE);
        break;

    case TMB_FUNCTION_READ_HOLDING_REGISTERS:
        TMB_ON_FALSE_RETURN(TMB_READ_HOLDING_REGISTER_MIN_QUANTITY <= request->read_holding_registers.quantity &&
                                    request->read_holding_registers.quantity <= TMB_READ_HOLDING_REGISTER_MAX_QUANTITY,
                            TMB_E_ILLEGAL_DATA_VALUE);
        break;
    case TMB_FUNCTION_READ_INPUT_REGISTERS:
        TMB_ON_FALSE_RETURN(TMB_READ_INPUT_REGISTER_MIN_QUANTITY <= request->read_input_registers.quantity &&
                                    request->read_holding_registers.quantity <= TMB_READ_HOLDING_REGISTER_MAX_QUANTITY,
                            TMB_E_ILLEGAL_DATA_VALUE);
        break;
    case TMB_FUNCTION_WRITE_SINGLE_COIL:
        TMB_ON_FALSE_RETURN(request->write_single_coil.value == TMB_WRITE_SINGLE_COIL_TRUE_VALUE ||
                                    request->write_single_coil.value == TMB_WRITE_SINGLE_COIL_FALSE_VALUE,
                            TMB_E_ILLEGAL_DATA_VALUE);
        break;
    case TMB_FUNCTION_WRITE_SINGLE_REGISTER:
        break;
    case TMB_FUNCTION_WRITE_MULTIPLE_COILS:
        TMB_ON_FALSE_RETURN(TMB_WRITE_MULTIPLE_COILS_MIN_QUANTITY <= request->write_multiple_coils.quantity &&
                                    request->write_multiple_coils.quantity <= TMB_WRITE_MULTIPLE_COILS_MAX_QUANTITY,
                            TMB_E_ILLEGAL_DATA_VALUE);
        TMB_ON_FALSE_RETURN(request->write_multiple_coils.byte_count ==
                                    request->write_multiple_coils.quantity / 8 +
                                            (request->write_multiple_coils.quantity % 8 == 0),
                            TMB_E_ILLEGAL_DATA_VALUE);
        break;
    case TMB_FUNCTION_WRITE_MULTIPLE_REGISTERS:
        TMB_ON_FALSE_RETURN(TMB_WRITE_MULTIPLE_REGISTERS_MIN_QUANTITY <= request->write_multiple_registers.quantity &&
                                    request->write_multiple_registers.quantity <= TMB_WRITE_MULTIPLE_COILS_MAX_QUANTITY,
                            TMB_E_ILLEGAL_DATA_VALUE);
        TMB_ON_FALSE_RETURN(request->write_multiple_registers.byte_count ==
                                    request->write_multiple_registers.quantity * 2,
                            TMB_E_ILLEGAL_DATA_VALUE);
        break;

    /* codes not implemented */
    case TMB_FUNCTION_READ_EXCEPTION_STATUS:
    case TMB_FUNCTION_DIAGNOSTIC:
    case TMB_FUNCTION_GET_COM_EVENT_COUNTER:
    case TMB_FUNCTION_GET_COM_EVENT_LOG:
    case TMB_FUNCTION_REPORT_SLAVE_ID:
    case TMB_FUNCTION_READ_FILE_RECORD:
    case TMB_FUNCTION_WRITE_FILE_RECORD:
    case TMB_FUNCTION_MASK_WRITE_REGISTER:
    case TMB_FUNCTION_READ_WRITE_MULTIPLE_REGISTERS:
    case TMB_FUNCTION_READ_FIFO_QUEUE:
    case TMB_FUNCTION_ENCAPSULATED_TRANSPORT:
    default:
        return TMB_E_ILLEGAL_FUNCTION;
    }

    return TMB_SUCCESS;
}

tmb_error_t tmb_client_send_request(tmb_handle_t *handle, const tmb_request_pdu_t *request,
                                    tmb_response_pdu_t *response) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(request != NULL, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(response != NULL, TMB_E_INVALID_ARGUMENTS);

    /* pre-validate the request, to avoid sending invalid requests to the server */
    TMB_ERROR_CHECK(tmb_request_validate(request));

    /* constructs request PDU */
    tmb_adu_t adu;
    TMB_ERROR_CHECK(tmb_adu_init(&adu, handle->buffer, handle->buffer_size, handle->encapsulation,
                                 handle->client.last_transaction_identifier++, handle->client.device_address));
    TMB_ERROR_CHECK(tmb_adu_serialize_request(&adu, request));
    TMB_ERROR_CHECK(tmb_adu_finalize(&adu));

    /* send request to transport */
    TMB_ERROR_CHECK(tmb_send(handle, adu.buffer, adu.size));

    /* the server here processed the response. Peek 2 bytes from the response
     * to know their status (success/failure) and size, then read the whole response */
    TMB_ERROR_CHECK(tmb_receive(handle, handle->buffer, TMB_RESPONSE_LOOKAHEAD_BYTES));

    if (TMB_IS_FUNCTION_EXCEPTION_CODE(handle->buffer[0])) {
        /* an exception occurred. Return an error, that is the exception code returned by the server */

        uint8_t exception_code = handle->buffer[1];
        if (exception_code != 0) {
            return (tmb_error_t)exception_code;
        }

        /* function code says an error is occurred, but exception code is 0. Should not happen! */
        return TMB_FAILURE;
    }

    size_t response_size = tmb_get_response_size(handle->buffer[0], handle->buffer[1]);
    if (response_size > TMB_RESPONSE_LOOKAHEAD_BYTES) {
        TMB_ON_FALSE_RETURN(response_size <= handle->buffer_size, TMB_E_NO_MEMORY);

        /* read remaining bytes */
        TMB_ERROR_CHECK(tmb_receive(handle, handle->buffer + TMB_RESPONSE_LOOKAHEAD_BYTES,
                                    response_size - TMB_RESPONSE_LOOKAHEAD_BYTES));
    }

    /* now I have the whole response in the buffer. Need to parse it. */
    TMB_ERROR_CHECK(tmb_response_parse(response, handle->buffer, response_size));

    return TMB_SUCCESS;
}

tmb_error_t tmb_client_set_device_address(tmb_handle_t *handle, uint8_t address) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_CLIENT, TMB_E_INVALID_MODE);

    handle->client.device_address = address;

    return TMB_SUCCESS;
}

tmb_error_t tmb_read_coils(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity, uint8_t *values) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(values != NULL, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_CLIENT, TMB_E_INVALID_MODE);

    tmb_request_pdu_t request = {
        .function_code = TMB_FUNCTION_READ_COILS,
        .read_coils = {
            .start_address = start_address,
            .quantity = quantity,
        },
    };

    tmb_response_pdu_t response;
    TMB_ERROR_CHECK(tmb_client_send_request(handle, &request, &response));

    memcpy(values, response.read_coils.coil_status, response.read_coils.byte_count);

    return TMB_SUCCESS;
}

tmb_error_t tmb_read_discrete_inputs(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity, uint8_t *values) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(values != NULL, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_CLIENT, TMB_E_INVALID_MODE);

    tmb_request_pdu_t request = {
        .function_code = TMB_FUNCTION_READ_DISCRETE_INPUTS,
        .read_discrete_inputs = {
            .start_address = start_address,
            .quantity = quantity,
        },
    };

    tmb_response_pdu_t response;
    TMB_ERROR_CHECK(tmb_client_send_request(handle, &request, &response));

    memcpy(values, response.read_discrete_inputs.input_status, response.read_discrete_inputs.byte_count);

    return TMB_SUCCESS;
}

tmb_error_t tmb_read_holding_registers(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity,
                                       uint16_t *values) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(values != NULL, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_CLIENT, TMB_E_INVALID_MODE);

    tmb_request_pdu_t request = {
        .function_code = TMB_FUNCTION_READ_HOLDING_REGISTERS,
        .read_holding_registers = {
            .start_address = start_address,
            .quantity = quantity,
        },
    };

    tmb_response_pdu_t response;
    TMB_ERROR_CHECK(tmb_client_send_request(handle, &request, &response));

    memcpy(values, response.read_holding_registers.register_values, response.read_holding_registers.byte_count);

    return TMB_SUCCESS;
}

tmb_error_t tmb_read_input_registers(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity,
                                     uint16_t *values) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(values != NULL, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_CLIENT, TMB_E_INVALID_MODE);

    tmb_request_pdu_t request = {
        .function_code = TMB_FUNCTION_READ_INPUT_REGISTERS,
        .read_input_registers = {
            .quantity = quantity,
            .start_address = start_address,
        },
    };

    tmb_response_pdu_t response;
    TMB_ERROR_CHECK(tmb_client_send_request(handle, &request, &response));

    memcpy(values, response.read_input_registers.register_values, response.read_holding_registers.byte_count);

    return TMB_SUCCESS;
}

tmb_error_t tmb_write_single_coil(tmb_handle_t *handle, uint16_t address, uint16_t value) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_CLIENT, TMB_E_INVALID_MODE);

    tmb_request_pdu_t request = {
        .function_code = TMB_FUNCTION_WRITE_SINGLE_COIL,
        .write_single_coil = {
            .address = address,
            .value = value,
        },
    };

    tmb_response_pdu_t response;
    TMB_ERROR_CHECK(tmb_client_send_request(handle, &request, &response));

    return TMB_SUCCESS;
}

tmb_error_t tmb_write_single_register(tmb_handle_t *handle, uint16_t address, uint16_t value) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_CLIENT, TMB_E_INVALID_MODE);

    tmb_request_pdu_t request = {
        .function_code = TMB_FUNCTION_WRITE_SINGLE_REGISTER,
        .write_single_register = {
            .address = address,
            .value = value,
        },
    };

    tmb_response_pdu_t response;
    TMB_ERROR_CHECK(tmb_client_send_request(handle, &request, &response));

    return TMB_SUCCESS;
}

tmb_error_t tmb_write_multiple_coils(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity,
                                     const uint8_t *values) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_CLIENT, TMB_E_INVALID_MODE);

    tmb_request_pdu_t request = {
        .function_code = TMB_FUNCTION_WRITE_MULTIPLE_COILS,
           .write_multiple_coils = {
            .start_address = start_address,
            .quantity = quantity,
            .byte_count = quantity,
            .values = values,
        },
    };

    tmb_response_pdu_t response;
    TMB_ERROR_CHECK(tmb_client_send_request(handle, &request, &response));

    return TMB_SUCCESS;
}

tmb_error_t tmb_write_multiple_registers(tmb_handle_t *handle, uint16_t start_address, uint16_t quantity,
                                         const uint16_t *values) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_CLIENT, TMB_E_INVALID_MODE);

    tmb_request_pdu_t request = {
        .function_code = TMB_FUNCTION_WRITE_MULTIPLE_REGISTERS,
        .write_multiple_registers = {
            .start_address = start_address,
            .quantity = quantity,
            .byte_count = quantity,
            .values = values,
        },
    };

    tmb_response_pdu_t response;
    TMB_ERROR_CHECK(tmb_client_send_request(handle, &request, &response));

    return TMB_SUCCESS;
}

tmb_error_t tmb_server_set_callback(tmb_handle_t *handle, uint16_t address, const tmb_callbacks_t *callbacks) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(address <= TMB_ADDRESS_ANY, TMB_E_INVALID_ARGUMENTS);
    TMB_ON_FALSE_RETURN(handle->mode == TMB_MODE_SERVER, TMB_E_INVALID_MODE);

    size_t first_available_idx = SIZE_MAX;
    for (size_t i = 0; i < TMB_SERVER_MAX_ADDRESSES; i++) {
        if (handle->server.callbacks[i].address == address) {
            /* replace existing entry */
            handle->server.callbacks[i].callbacks = callbacks;

            return TMB_SUCCESS;
        }

        if (handle->server.callbacks[i].callbacks == NULL) {
            /* record first existing entry*/
            first_available_idx = i;
        }
    }

    if (first_available_idx < TMB_SERVER_MAX_ADDRESSES) {
        /* write to the first existing entry */
        handle->server.callbacks[first_available_idx].address = address;
        handle->server.callbacks[first_available_idx].callbacks = callbacks;

        return TMB_SUCCESS;
    }

    /* no slot available found */
    return TMB_E_NO_MEMORY;
}

tmb_error_t tmb_server_run_iteration(tmb_handle_t *handle) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);

    return TMB_E_NOT_IMPLEMENTED;
}

tmb_error_t tmb_server_run_forever(tmb_handle_t *handle) {
    TMB_ON_FALSE_RETURN(handle != NULL && handle->is_valid, TMB_E_INVALID_ARGUMENTS);

    while (true) {
        tmb_server_run_iteration(handle);
    }
}

#ifdef TMB_POSIX_SUPPORTED

#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

typedef struct {
    int fd;
} tmb_posix_transport_t;

static int tmb_posix_transport_read(void *user_ctx, uint8_t *buffer, size_t nbytes) {
    tmb_posix_transport_t *transport = user_ctx;

    return read(transport->fd, buffer, nbytes);
}

static int tmb_posix_transport_write(void *user_ctx, const uint8_t *buffer, size_t nbytes) {
    tmb_posix_transport_t *transport = user_ctx;

    return write(transport->fd, buffer, nbytes);
}

tmb_transport_t *tmb_posix_transport_new_tcpip(const char *hostname, uint16_t port) {
    tmb_transport_t *transport = NULL;
    tmb_posix_transport_t *posix_transport = NULL;

    posix_transport = calloc(sizeof(tmb_posix_transport_t), 1);
    if (posix_transport == NULL) {
        goto error;
    }

    transport = malloc(sizeof(tmb_transport_t));
    if (transport == NULL) {
        goto error;
    }
    transport->user_data = posix_transport;
    transport->read = tmb_posix_transport_read;
    transport->write = tmb_posix_transport_write;

    struct hostent *hostent = gethostbyname(hostname);
    if (hostent == NULL) {
        goto error;
    }

    posix_transport->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (posix_transport->fd <= 0) {
        goto error;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {
            .s_addr = *((in_addr_t *)hostent->h_addr_list[0]),
        },
    };

    if (connect(posix_transport->fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        goto error;
    }

    return transport;

error:
    free(transport);
    free(posix_transport);

    return NULL;
}

tmb_transport_t *tmb_posix_transport_new_serial(const char *device, uint16_t baudrate, uint8_t stop_bits,
                                                tmb_serial_parity_t parity) {
    return NULL;
}

void tmb_posix_transport_free(tmb_transport_t *transport) {
    if (transport == NULL) {
        return;
    }

    if (transport->user_data != NULL) {
        tmb_posix_transport_t *transport_ctx = transport->user_data;
        close(transport_ctx->fd);
        free(transport->user_data);
    }

    free(transport);
}

#endif /* TMB_POSIX_SUPPORTED */

#endif /* TMB_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TINYMODBUS_H */