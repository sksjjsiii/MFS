
#include "webfast_runtime.h"
#include <stdio.h>

int main() {
    printf("Testing JSON Builder\n");
    printf("====================\n\n");
    
    JsonBuilder jb;
    jb_init(&jb);
    
    printf("1. Starting object...\n");
    jb_start_object(&jb);
    printf("   Buffer: %s\n", jb_get(&jb));
    printf("   pos=%d, first_key=%d, depth=%d\n", jb.pos, jb.first_key, jb.depth);
    
    printf("\n2. Adding key 'simple_array'...\n");
    jb_key(&jb, "simple_array");
    printf("   Buffer: %s\n", jb_get(&jb));
    printf("   pos=%d, first_key=%d, depth=%d\n", jb.pos, jb.first_key, jb.depth);
    printf("   Last char: '%c' (ASCII %d)\n", jb.buffer[jb.pos - 1], jb.buffer[jb.pos - 1]);
    
    printf("\n3. Starting array...\n");
    jb_start_array(&jb);
    printf("   Buffer: %s\n", jb_get(&jb));
    printf("   pos=%d, first_key=%d, depth=%d\n", jb.pos, jb.first_key, jb.depth);
    
    printf("\n4. Adding integers...\n");
    jb_arr_int(&jb, 1);
    jb_arr_int(&jb, 2);
    jb_arr_int(&jb, 3);
    printf("   Buffer: %s\n", jb_get(&jb));
    
    printf("\n5. Ending array...\n");
    jb_end_array(&jb);
    printf("   Buffer: %s\n", jb_get(&jb));
    
    printf("\n6. Ending object...\n");
    jb_end_object(&jb);
    printf("   Buffer: %s\n", jb_get(&jb));
    
    printf("\n====================\n");
    printf("Final JSON: %s\n", jb_get(&jb));
    printf("====================\n");
    
    return 0;
}
