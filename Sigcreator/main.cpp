#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
 
int main() {
    int i, len, out_i = 0, mask_i = 0;
    char *sig, *out, *mask;
 
    sig = (char*)calloc(1024, sizeof(char));
 
    printf("?");
    fgets(sig, 1022, stdin);
    len = strlen(sig);
 
    out = (char*)calloc(len*2, sizeof(char));
    mask = (char*)calloc(len*2, sizeof(char));
 
    for(i = 0;i < len;i++) {
        if(isalnum(sig[i]) && isalnum(sig[i+1])) {
            out[out_i++] = '\\';
            out[out_i++] = 'x';
            out[out_i++] = sig[i];
            out[out_i++] = sig[i+1];
            if(sig[i+2] == '?') {
                mask[mask_i++] = '?';
                i += 2;
            }
            else {
                mask[mask_i++] = 'x';
                i++;
            }
        }
    }
 
    printf("\n\nSize%d\n\nSig:\n%s\n\nMask:\n%s", strlen(mask), out, mask);
    getchar();
 
    free(sig);
    free(out);
    free(mask);
    return 0;
}