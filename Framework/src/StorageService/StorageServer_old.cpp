#if 0
    while (1) {
        uint8_t op;
        untrusted_util::socket_receive(socket, &op, sizeof(uint8_t));

        switch (op) {
            case OP_UEE_INIT: {
                // clean previous elements, if any
                //repository_clear();

                printf("Server init\n");
                //memcpy(&l_size, in_buffer + sizeof(uint8_t), sizeof(size_t));
                //memcpy(&d_size, in_buffer + sizeof(uint8_t) + sizeof(size_t), sizeof(size_t));
                //TODO clean previous map data
                break;
            }
            case OP_UEE_ADD: {
                //printf("Add image (c. %lu)\n", I.size());

                gettimeofday(&start, NULL);

                size_t nr_labels;
                untrusted_util::socket_receive(socket, &nr_labels, sizeof(size_t));
                //printf("nr labels %d\n", nr_labels);
                /*for (int j = 0; j < in_len - 10; ++j) {
                    if(j < in_len - 13 && tmp[j] == 0xAE && tmp[j+1] == 0xD8 && tmp[j+2] == 0xD2) {
                        printf("this has it %d\n", j - 9);
                    }
                }*/

                uint8_t* buffer = (uint8_t*)malloc(nr_labels * (l_size + d_size));
                untrusted_util::socket_receive(socket, buffer, nr_labels * (l_size + d_size));

                for (size_t i = 0; i < nr_labels; ++i) {
                    void* l = buffer + i * (l_size + d_size);
                    void* d = buffer + i * (l_size + d_size) + l_size;

                    I[l] = d;
                }

                gettimeofday(&end, NULL);
                total_add_time += untrusted_util::time_elapsed_ms(start, end);

                break;
            }
            case OP_UEE_SEARCH: {
                gettimeofday(&start, NULL);

                size_t nr_labels;
                untrusted_util::socket_receive(socket, &nr_labels, sizeof(size_t));

                unsigned char* label = new unsigned char[l_size * nr_labels];
                untrusted_util::socket_receive(socket, label, l_size * nr_labels);

                size_t res_len = nr_labels * d_size;
                uint8_t* buffer = (uint8_t*)malloc(res_len);

                for (size_t i = 0; i < nr_labels; ++i) {
                    if(!I[label + i * l_size]) {
                        printf("Map error: %lu/%lu\n", i, nr_labels);
                        untrusted_util::debug_printbuf(label, l_size);
                        exit(1);
                    }

                    memcpy(buffer + i * d_size, I[label + i * l_size], d_size);
                }

                gettimeofday(&end, NULL);
                total_search_time += untrusted_util::time_elapsed_ms(start, end);

                // send response
                untrusted_util::socket_send(socket, buffer, res_len);
                free(buffer);
                break;
            }
            case OP_UEE_READ_MAP: {
                printf("Reading map from disk\n");
                printf("functionality disabled\n");
                /*FILE* map_file = fopen("map_data", "rb");
                if(!map_file){
                    printf("Error opening map file\n!");
                    exit(1);
                }

                size_t nr_pairs;
                fread(&nr_pairs, sizeof(size_t), 1, map_file);
                printf("nr_pairs %lu\n", nr_pairs);

                int count = 0, ccount = 0;
                for(size_t i = 0; i < nr_pairs; ++i) {
                    void* label = (uint8_t*)malloc(l_size);

                    uint8_t tmp_a[l_size]; // TODO this needs to vanish, fread directly into malloc'd buffers was not working
                    fread(tmp_a, l_size, 1, map_file);
                    memcpy(label, tmp_a, l_size);

                    //untrusted_util::debug_printbuf((uint8_t*)label, l_size);

                    void* d = (uint8_t*)malloc(d_size);
                    uint8_t tmp_b[d_size];
                    fread(tmp_b, d_size, 1, map_file);
                    memcpy(d, tmp_b, d_size);

                    if(((uint8_t*)label)[0] == 0x56 && ((uint8_t*)label)[1] == 0x97 && ((uint8_t*)label)[5] == 0xfa)
                        count++;
                    else ccount++;


                    I[label] = d;
                }
                //printf("bad count %d; good count %d\n", count, ccount);
                fclose(map_file);
                printf("done!\n");*/
                break;
            }
            case OP_UEE_WRITE_MAP: {
                printf("Writing map to disk\n");
                printf("functionality disabled\n");
                /*FILE* map_file = fopen("map_data", "wb");
                if(!map_file){
                    printf("Error opening map file\n!");
                    exit(1);
                }

                const size_t nr_pairs = I.size();
                fwrite(&nr_pairs, sizeof(size_t), 1, map_file);

                int count = 0, ccount = 0;

                for (tbb::concurrent_unordered_map<void*, void*, VoidHash<l_size>, VoidEqual<l_size>>::iterator it = I.begin(); it != I.end() ; ++it) {
                    if(!((uint8_t*)it->first)[0] && !((uint8_t*)it->first)[1] && !((uint8_t*)it->first)[5])
                        count++;
                    else ccount++;

                    fwrite(it->first, sizeof(l_size), 1, map_file);
                    fwrite(it->second, sizeof(d_size), 1, map_file);
                }

                printf("bad count %d; good count %d\n", count, ccount);
                fclose(map_file);
                printf("done!\n");*/
                break;
            }
            case OP_UEE_DUMP_BENCH: {
                printf("-- STORAGE BENCHMARK --\n");
                printf("-- VISEN index size: %lu--\n", I.size());
                printf("-- VISEN storage TOTAL add : %lf ms--\n", total_add_time);
                printf("-- VISEN storage TOTAL search : %lf ms --\n", total_search_time);
                break;
            }
            case OP_UEE_CLEAR: {
                printf("Clear\n");
                repository_clear();
                break;
            }
            default:
                printf("SseServer unkonwn command: %02x\n", op);
        }
    }
#fi
