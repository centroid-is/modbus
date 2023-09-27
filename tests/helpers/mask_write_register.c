// Little helper program to get a known good
// reference implementation of mask_write_register
// Unable to make mbpoll cmdline tool
// generate this request.
#include <stdio.h>
#include <modbus.h>
#include <errno.h>

int main(void){
    modbus_t *mb;

    mb = modbus_new_tcp("127.0.0.1", 502);

    if (modbus_connect(mb) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(mb);
        return -1;
    }
    uint16_t and_mask = 15;
    uint16_t or_mask  = 16;
    modbus_mask_write_register(mb, 14, and_mask, or_mask);

    modbus_close(mb);
    modbus_free(mb);

    printf("Done!\n");
    return 0;
}
