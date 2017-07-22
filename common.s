.section .text
.global fastcp

.type fastcp, @function
    # void fastcp(char *dest, char *src, uint32_t count)
fastcp:
    pushl   %esi
    pushl   %edi
    movl    12(%esp), %edi # dest
    movl    16(%esp), %esi # src
    movl    20(%esp), %ecx # count
    rep     movsb
    popl    %edi
    popl    %esi
    ret
