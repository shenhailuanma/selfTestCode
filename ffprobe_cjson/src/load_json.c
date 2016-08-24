#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "json-c/json.h"


int check_stream_codec(char * filepath, char * codec_name)
{

    int i;
    int len;
    int result = 0;
    struct json_object * json_root;
    struct json_object * json_streams;
    struct json_object * json_stream_one;
    struct json_object * json_stream_type;
    struct json_object * json_stream_codec;

    json_root = json_object_from_file(filepath);
    if(json_root){
        //printf("%s\n", json_object_to_json_string_ext(json_root, JSON_C_TO_STRING_PRETTY));

        // get json info
        json_streams = json_object_object_get(json_root, "streams");
        if(json_streams){
            //printf("%s\n", json_object_to_json_string_ext(json_streams, JSON_C_TO_STRING_PRETTY));

            // the 'json_streams' should array type
            if(json_object_is_type(json_streams, json_type_array)){

                len = json_object_array_length(json_streams);

                for(i = 0; i < len; i++){
                    //printf("stream[%d]:", i);
                    json_stream_one = json_object_array_get_idx(json_streams, i);
                    if(json_stream_one){
                        //printf("%s\n", json_object_to_json_string_ext(json_stream_one, JSON_C_TO_STRING_PRETTY));

                        json_stream_type = json_object_object_get(json_stream_one, "codec_type");
                        if(json_stream_type){
                            //printf("codec_type:%s\n", json_object_to_json_string_ext(json_stream_type, JSON_C_TO_STRING_PRETTY));
                            
                            if(strcmp("video", json_object_get_string(json_stream_type)) == 0){
                                json_stream_codec = json_object_object_get(json_stream_one, "codec_name");
                                if(json_stream_codec){
                                    //printf("stream[%d], codec_name:%s\n", i, json_object_to_json_string_ext(json_stream_codec, JSON_C_TO_STRING_PRETTY));
                                    if(strcmp(json_object_get_string(json_stream_codec), codec_name) == 0){
                                        result = 1;
                                    }
                                }else{
                                    result = 0;
                                }
                            }
                        }
                    }
                }

            }


        }else{
            printf("not found key 'streams'\n");
        }

        json_object_put(json_root);
    }




    return result;
}


int main(int argc, char ** argv)
{
    int i;

    for(i = 0; i < argc; i++)
        printf("argv[%d]:%s\n", i, argv[i]);

    if(argc < 2){
        printf("no filepath, return.\n");
        return 1;
    }

    int ret;

    for(i = 0; i < 1000000000; i++){
        ret = check_stream_codec(argv[1], "h264");
        printf("check_stream_codec return:%d\n", ret);
    }


    return 0;
}