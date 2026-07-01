#include <stdio.h>
#include <string.h>

#include "objec_store.h"
#include "blob.h"

int main(void) {
    printf("Gitlite Toolchain Ready.\n\n");

    //basic store and retrieve
    ObjectStore* os = object_store_init();

    const char* content1 = "int main() { return 0; }";
    Blob* b1 = blob_create(content1, strlen(content1));
    object_store_put(os, "a1b2c3d4", b1);

    Blob* retrieved = object_store_get(os, "a1b2c3d4");
    if (retrieved) {
        printf("[PASS] Retrieved blob: \"%s\"\n", retrieved->content);
    } else {
        printf("[FAIL] Blob Not Found");
    }


                 //miss on unknown key
    Blob* missing = object_store_get(os, "deadbeaf");
    if (!missing) {
        printf("[PASS] Unknown key correctly returned NULL\n");
    }

            //collision handling
    HashMap* tiny = hash_map_init(1);
    Blob* b2 = blob_create("file A", 6);
    Blob* b3 = blob_create("file B", 6);
    hashmap_put(tiny, "key_alpha", b2);
    hashmap_put(tiny, "key_beta", b3);

    Blob* ra = (Blob*)hashmap_get(tiny, "key_alpha");
    Blob* rb = (Blob*)hashmap_get(tiny, "key_beta");
    printf("[%s] Collision test - key_alpha: \"%s\"\n",
            (ra && strcmp(ra->content, "file A") == 0) ? "PASS" : "FAIL",
            ra ? ra->content : "NULL");
    printf("[%s] Collision test - key_beta: \"%s\"\n",
            (rb && strcmp(rb->content, "file B") == 0) ? "PASS" : "FAIL",
            rb ? rb->content : "NULL");


        //cleanup
    blob_free(b2);
    blob_free(b3);
    hashmap_free(tiny);
    object_store_free(os);

    printf("\nAll tests passed.\n");

    return 0;

}