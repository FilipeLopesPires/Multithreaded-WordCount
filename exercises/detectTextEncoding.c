#include <stdio.h>

int main (void) {
    char word[] = "á";
    char hexWord[16];
    char binWord[100];
    
    printf("%x\n",word[0]); // if c3 then UTF-8, if e1 then ISO (...)

    int n = sprintf(hexWord, "%x", word[0]);
    printf("%s\n",hexWord);

    if (word[0]==0xffffffc3) {
       printf("yes");
    }

    return(0);
}