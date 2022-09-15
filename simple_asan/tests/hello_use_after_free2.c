#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char *x = malloc(80);
    for (size_t i = 0; i < 80; i += 5) {
        memcpy(&x[i], "HELO", 5);
    }
    printf("%s\n", &x[40]);
    free(x);
    printf("%s\n", &x[40]);
}

// 11

// stdout: HELO

// stderr: Invalid heap access: address 0x100001fd8 is not in an allocation or was already freed
// stderr: at /lib/x86_64-linux-gnu/libc.so.6(+0x1885f5)
// stderr: __strlen_avx2
// stderr: strlen-avx2.S:65
// stderr: at /lib/x86_64-linux-gnu/libc.so.6(+0x78d45)
// stderr: __vfprintf_internal
// stderr: vfprintf-internal.c:1688 (discriminator 87)
// stderr: at /lib/x86_64-linux-gnu/libc.so.6(_IO_printf+0xaf)
// stderr: ??
// stderr: ??:0
// stderr: at bin/hello_use_after_free2(main+0xab)
// stderr: main
// stderr: hello_use_after_free2.c:13

// modified above....
