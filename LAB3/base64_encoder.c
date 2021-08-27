#include<stdio.h>
#include<stdlib.h>
#include<string.h>
char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};

char *encode(const char *original_message,unsigned int input_length) {
    unsigned int output_length = 4 * ((input_length + 2) / 3);
    

    char *encoded_message = (char *)malloc(sizeof(char)*(output_length+1));
    // printf("%d\n", (int)output_length);
    if (encoded_message == NULL) return NULL;

    unsigned int i=0;
    unsigned int j=0;
    for (; i < input_length;) {

        unsigned int octet_a = 0, octet_b = 0, octet_c = 0, triple;
        if(i < input_length){
           octet_a = (unsigned char)original_message[i++];
        }
        if(i < input_length){
           octet_b = (unsigned char)original_message[i++];
        }
        if(i < input_length){
           octet_c = (unsigned char)original_message[i++];
        }

        triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        if(j < output_length){encoded_message[j++] = encoding_table[(triple >> 18)];} //right shift by 18 bits
        triple = (triple&((1<<18)-1));
        if(j < output_length){encoded_message[j++] = encoding_table[(triple >> 12)]; }// right shift by 12 bits
        triple = (triple&((1<<12)-1));
        if(j < output_length){encoded_message[j++] = encoding_table[(triple >> 6)]; }// right shift by 6 bits
        triple = (triple&((1<<6)-1));
        if(j < output_length){encoded_message[j++] = encoding_table[(triple)];} //no shift
    }

    if(input_length%3 != 0){
        encoded_message[output_length-1] = '=';
    }
    if(input_length%3 == 1){
        encoded_message[output_length-2] = '=';
    }
    encoded_message[output_length] = '\0';
    return encoded_message;
}