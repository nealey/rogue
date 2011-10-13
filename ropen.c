/*
 * print out an encrypted password on the standard output
 *
 * @(#)ropen.c	1.4 (Berkeley) 02/05/99
 */
#include <stdio.h>

int
main(void)
{
    static char buf[80];

    fprintf(stderr, "Password: ");
    fgets(buf, 80, stdin);
    buf[strlen(buf) - 1] = '\0';
    printf("%s\n", crypt(buf, "mT"));
}
