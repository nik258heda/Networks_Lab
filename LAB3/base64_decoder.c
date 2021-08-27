#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//Base64_decoder
char *decoding_table = NULL;
char encoding_table_[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                            'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                            'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                            'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                            'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                            'w', 'x', 'y', 'z', '0', '1', '2', '3',
                            '4', '5', '6', '7', '8', '9', '+', '/'};
void build_decoding_table() {
    decoding_table = (char *)malloc(sizeof(char)*256);
    for (int i = 0; i < 64; i++){
        decoding_table[(unsigned char) encoding_table_[i]] = i;
    }
}
unsigned char *decode(const char *original_message,unsigned int original_length) {
    if (decoding_table == NULL) build_decoding_table();
 
    if (original_length % 4 != 0) return NULL;
 
    unsigned int decoded_length = (original_length / 4) * 3;
    if (original_message[original_length - 1] == '=') {
        (decoded_length)--;
    }
    if (original_message[original_length - 2] == '=') {
        (decoded_length)--;
    }
 
    unsigned char *decoded_message = (unsigned char *)malloc(sizeof(unsigned char)*decoded_length);
    if (decoded_message == NULL) return NULL;
 
    for (unsigned int i = 0, j = 0; i < original_length;) {
 
        unsigned int sextet_a = 0, sextet_b = 0, sextet_c = 0, sextet_d = 0;
        if(original_message[i] != '='){
            sextet_a = decoding_table[original_message[i]];
        }
        i++;
        if(original_message[i] != '='){
            sextet_b = decoding_table[original_message[i]];
        }
        i++;
        if(original_message[i] != '='){
            sextet_c = decoding_table[original_message[i]];
        }
        i++;
        if(original_message[i] != '='){
            sextet_d = decoding_table[original_message[i]];
        }
        i++;
        unsigned int triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + (sextet_d);
 
        if (j < decoded_length) {
            decoded_message[j++] = (triple >> 16);
            triple = (triple&((1<<16)-1));
        }
        if (j < decoded_length) {
            decoded_message[j++] = (triple >> 8);
            triple = (triple&((1<<8)-1));
        }
        if (j < decoded_length) {
            decoded_message[j++] = (triple);
        }
    }
    return decoded_message;
}