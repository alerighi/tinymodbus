/**
 * \file tinymodbus.h
 * \brief Tiny Modbus - A small, header-only, portable and compliant
 *      Modbus implementation, suitable for embedded systems.
 * \copyright Copyright (c) 2024
 * \author Alessandro Righi <alessandro.righi@alerighi.it>
 */

#ifndef TINYMODBUS_H
#define TINYMODBUS_H

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

    /** Operation failed without a specific error cause */
    TMB_FAILURE = -1,

    /**
     * Operation was ignored. Returning this codes from a callback
     * causes no response (neither success or failure) to be sent to
     * the client. Useful when listening on any address
     */
    TMB_IGNORED = -2,

    /* The current operation timed out */
    TMB_ERROR_TIMEOUT = -3,

    /** An invalid argument has be provided to a function */
    TMB_ERROR_INVALID_ARGUMENTS = -4,

    /** The operation cannot be performed in the specified mode of operation
     * (i.e. requesting a client function when in server mode)
     */
    TMB_ERROR_INVALID_MODE = -5,

    /** The requested functionality is not implemented */
    TMB_ERROR_NOT_IMPLEMENTED = -6,

    /** There is no enough memory available to complete the operation */
    TMB_ERROR_NO_MEMORY = -7,
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
 * \typedef tmb_handle_t
 * \brief An handle to the Tiny Modbus instance. This is here only
 *      to allow for static allocation of the handle. It must be
 *      treated as a black-box.
 */
typedef struct {
    /** true if the current handle is valid (initialized) */
    bool is_valid;

    /** Current mode of operation */
    tmb_mode_t mode;

    /** Current encapsulation */
    tmb_encapsulation_t encapsulation;

    /** Current transport */
    const tmb_transport_t *transport;

    union {
        /** client-specific state */
        struct {
            uint8_t dummy;
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

/**
 * \typedef tmb_function_t
 * \brief Modbus function code definition
 * \note Refer to the Modbus protocol specification for their explanation
 */
typedef enum {
    TMB_FUNCTION_READ_COILS = 1,
    TMB_FUNCTION_READ_DISCRETE_INPUT = 2,
    TMB_FUNCTION_READ_HOLDING_REGISTER = 3,
    TMB_FUNCTION_READ_INPUT_REGISTER = 4,
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
tmb_error_t tmb_init(tmb_handle_t *handle, tmb_mode_t mode, tmb_encapsulation_t encapsulation,
                     const tmb_transport_t *transport);

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

/* In only one C file, define this macro to include the implementation code */
#ifdef TMB_IMPLEMENTATION

#include <string.h>

/* private macro definitions */
#define ERROR_CHECK(check, error) \
    do {                          \
        if (!(check)) {           \
            return (error);       \
        }                         \
    } while (false)

/* private types */

/* private functions */

/* public functions implementation */

tmb_error_t tmb_init(tmb_handle_t *handle, tmb_mode_t mode, tmb_encapsulation_t encapsulation,
                     const tmb_transport_t *transport) {
    ERROR_CHECK(handle != NULL, TMB_ERROR_INVALID_ARGUMENTS);
    memset(handle, 0, sizeof(tmb_handle_t));

    ERROR_CHECK(transport != NULL, TMB_ERROR_INVALID_ARGUMENTS);
    ERROR_CHECK(transport->read != NULL, TMB_ERROR_INVALID_ARGUMENTS);
    ERROR_CHECK(transport->write != NULL, TMB_ERROR_INVALID_ARGUMENTS);
    ERROR_CHECK(encapsulation != TMB_ENCAPSULATION_ASCII, TMB_ERROR_NOT_IMPLEMENTED);

    handle->mode = mode;
    handle->encapsulation = encapsulation;
    handle->transport = transport;
    handle->is_valid = true;

    return TMB_SUCCESS;
}

tmb_error_t tmb_server_set_callback(tmb_handle_t *handle, uint16_t address, const tmb_callbacks_t *callbacks) {
    ERROR_CHECK(handle != NULL && handle->is_valid, TMB_ERROR_INVALID_ARGUMENTS);
    ERROR_CHECK(address <= TMB_ADDRESS_ANY, TMB_ERROR_INVALID_ARGUMENTS);
    ERROR_CHECK(handle->mode == TMB_MODE_SERVER, TMB_ERROR_INVALID_MODE);

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
    return TMB_ERROR_NO_MEMORY;
}

tmb_error_t tmb_server_run_iteration(tmb_handle_t *handle) {
    ERROR_CHECK(handle != NULL && handle->is_valid, TMB_ERROR_INVALID_ARGUMENTS);

    return TMB_ERROR_NOT_IMPLEMENTED;
}

tmb_error_t tmb_server_run_forever(tmb_handle_t *handle) {
    ERROR_CHECK(handle != NULL && handle->is_valid, TMB_ERROR_INVALID_ARGUMENTS);

    while (true) {
        tmb_server_run_iteration(handle);
    }
}

#endif /* TMB_IMPLEMENTATION */

#endif /* TINYMODBUS_H */