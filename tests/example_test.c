#include <stdarg.h>
#include <setjmp.h>
#include <stddef.h>
#include <cmocka.h>

#define TMB_IMPLEMENTATION
#include <tinymodbus.h>

static void test_init(void **state) {
    tmb_handle_t handle;
    tmb_transport_t dummy_transport = {
        /* just to pass verification of NULL pointer */
        .read = (void *)1,
        .write = (void *)1,
    };
    uint8_t dummy_buffer[10];

    assert_int_equal(tmb_init(&handle, TMB_MODE_CLIENT, TMB_ENCAPSULATION_RTU, dummy_buffer, sizeof(dummy_buffer),
                              &dummy_transport),
                     TMB_SUCCESS);
    assert_int_equal(handle.is_valid, true);
}

static void test_init_invalid_config(void **state) {
    tmb_handle_t handle;
    uint8_t dummy_buffer[10];

    assert_int_equal(tmb_init(&handle, TMB_MODE_CLIENT, TMB_ENCAPSULATION_RTU, dummy_buffer, sizeof(dummy_buffer),
                              NULL),
                     TMB_E_INVALID_ARGUMENTS);
    assert_int_equal(handle.is_valid, false);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_init),
        cmocka_unit_test(test_init_invalid_config),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
