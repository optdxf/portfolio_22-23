#include "packet.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "math_util.h"

uint64_t CLIENT_INPUT_PACKET_SIZE = 6;
uint64_t CAR_SELECTION_PACKET_SIZE = 2;

packet_t *parse_packet(const unsigned char *buffer, uint64_t size) {
    if (size <= 0) return NULL;
    packet_type_t type = *buffer;
    packet_t *p = NULL;
    switch (type) {
        case CLIENT_INPUT: {
            if (size != CLIENT_INPUT_PACKET_SIZE) break;
            //printf("read: %i %i %i %i %i \n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
            client_input_packet_t *packet = malloc(sizeof(*packet));
            packet->type = type;
            packet->data.up_held = (bool)buffer[1];
            packet->data.down_held = (bool)buffer[2];
            packet->data.left_held = (bool)buffer[3];
            packet->data.right_held = (bool)buffer[4];
            packet->data.space_held = (bool)buffer[5];
            p = (packet_t*)packet;
            break;
        }
        case LOBBY_LIST: {
            if (size <= 2) break;   // we expect the packet to have at least 3 bytes
            if (size != 2 + (int)buffer[1] * sizeof(int)) break;

            lobby_list_packet_t *packet = malloc(sizeof(*packet));
            packet->type = type;
            packet->client_count = (int)buffer[1];
            //printf("client count %i\n", packet->client_count);
            packet->clients = list_init(packet->client_count, free);
            for (size_t i = 0; i < packet->client_count; ++i) {
                client_lobby_info_t *lobby_info = malloc(sizeof(*lobby_info));
                lobby_info->client_id = *(int*)(buffer + 2 + (i * sizeof(int)));
                //printf("lobby id: %i\n", lobby_info->client_id);
                list_add(packet->clients, lobby_info);
            }
            p = (packet_t*)packet;
            break;
        }
        case START_MULTI_GAME: {
            if (size <= 3) break;   // we expect the packet to have at least 3 bytes
            if (size != 3 + sizeof(int) + ((int)buffer[2] * (sizeof(int) + 1))) break;

            start_multi_game_packet_t *packet = malloc(sizeof(*packet));
            packet->type = type;
            packet->map_type = (unsigned char)buffer[1];
            packet->client_count = (int)buffer[2];
            packet->local_client_id = *(int*)(buffer + 3);
            packet->car_info = list_init(packet->client_count, free);
            for (size_t i = 0; i < packet->client_count; ++i) {
                client_car_info_t *car_info = malloc(sizeof(*car_info));
                car_info->client_id = *(int*)(buffer + 3 + sizeof(int) + (i * (sizeof(int) + 1)));
                car_info->car_type = *(unsigned char*)(buffer + 3 + sizeof(int) + (i * (sizeof(int) + 1)) + sizeof(int));
                //printf("client_id: %i\n", car_info->client_id);
                //printf("car_type: %i\n", car_info->car_type);
                list_add(packet->car_info, car_info);
            }

            p = (packet_t*)packet;
            break;
        }
        case CAR_SELECTION: {
            if (size != CAR_SELECTION_PACKET_SIZE) {
                printf("incorrect size\n"); break;
            }
            car_selection_packet_t *packet = malloc(sizeof(*packet));
            packet->type = type;
            packet->car_type = buffer[1];
            p = (packet_t*)packet;
            break;
        }
        case UPDATE_ID: {
            if (size != 1 + sizeof(int)) break;
            update_client_id_packet_t *packet = malloc(sizeof(*packet));
            packet->type = type;
            packet->id = *(int*)(buffer + 1);
            p = (packet_t*)packet;
            break;
        }
        case MULTI_WINNER: {
            if (size != 1 + sizeof(int)) break;
            multi_winner_packet_t *packet = malloc(sizeof(*packet));
            packet->type = type;
            packet->id = *(int*)(buffer + 1);
            p = (packet_t*)packet;
            break;
        }
        case ALL_UPDATES: {
            if (size <= 2) break;
            size_t block_size = sizeof(int) + 3 * sizeof(double);
            if (size != 2 + (int)buffer[1] * block_size) {
                printf("uh oh!\n");
                printf("%zu size: \n", size);
                printf("expected: %zu\n", 2 + (int)buffer[1] * block_size);
            }

            all_car_body_packet_t *packet = malloc(sizeof(*packet));
            packet->type = type;
            packet->client_count = (int)buffer[1];
            //printf("initing size: %zu\n", packet->client_count);
            packet->car_body_data = list_init(packet->client_count, NULL);
            for (size_t i = 0; i < packet->client_count; ++i) {
                car_body_data_t *body_data = malloc(sizeof(*body_data));
                body_data->client_id = *(int*)(buffer + 2 + i * block_size);
                body_data->x =  *(double*)(buffer + 2 + i * block_size + sizeof(int));
                body_data->y = *(double*)(buffer + 2 + i * block_size + sizeof(int) + sizeof(double));
                body_data->angle = *(double*)(buffer + 2 + i * block_size + sizeof(int) + 2 * sizeof(double));

                //printf("[%zu] %zu %.9f %.9f %.9f\n", i, body_data->client_id, body_data->x, body_data->y, body_data->angle);
                list_add(packet->car_body_data, body_data);
            }
            p = (packet_t*)packet;
            break;
        }
        case UPDATE_LAPS: {
            if (size != 1 + 2 * sizeof(int)) break;
            update_lap_counter_packet_t *packet = malloc(sizeof(*packet));
            packet->type = type;
            packet->lap_count = *(int*)(buffer + 1);
            packet->lap_max = *(int*)(buffer + 1 + sizeof(int));
            printf("%i %i %i\n", packet->type, packet->lap_count, packet->lap_max);
            p = (packet_t*)packet;
            break;
        }
        default: break;
    }
    return p;
}

const unsigned char *write_packet(packet_t *p, packet_type_t type, uint64_t *size) {
    unsigned char *buffer = NULL;
    // first byte is always the enum
    switch (type) {
        case CLIENT_INPUT: {
            client_input_packet_t *packet = (client_input_packet_t*)p;
            *size = CLIENT_INPUT_PACKET_SIZE;
            buffer = malloc(*size);
            buffer[0] = type;
            buffer[1] = packet->data.up_held;
            buffer[2] = packet->data.down_held;
            buffer[3] = packet->data.left_held;
            buffer[4] = packet->data.right_held;
            buffer[5] = packet->data.space_held;
            //printf("write: %i %i %i %i %i \n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
            break;
        }
        case LOBBY_LIST: {
            lobby_list_packet_t *packet = (lobby_list_packet_t*)p;
            *size = 1 + 1 + packet->client_count * sizeof(int);
            buffer = malloc(*size);
            buffer[0] = type;
            buffer[1] = (unsigned char)packet->client_count;    // we know this will always be less than max val of uchar
            for (size_t i = 0; i < list_size(packet->clients); ++i) {
                *(int*)(buffer + 2 + (i * sizeof(int))) = ((client_lobby_info_t*)list_get(packet->clients, i))->client_id;
            }
            break;
        }
        case START_MULTI_GAME: {
            start_multi_game_packet_t *packet = (start_multi_game_packet_t*)p;
            *size = 3 + sizeof(int) + packet->client_count * (1+sizeof(int));
            buffer = malloc(*size);
            buffer[0] = type;
            buffer[1] = (unsigned char)packet->map_type;
            buffer[2] = (unsigned char)packet->client_count;    // we know this will always be less than max val of uchar
            *(int*)(buffer + 3) = packet->local_client_id;
            for (size_t i = 0; i < list_size(packet->car_info); ++i) {
                client_car_info_t *info = (client_car_info_t*)list_get(packet->car_info, i);
                *(int*)(buffer + 3 + sizeof(int) + i * (sizeof(int) + 1)) = info->client_id;
                buffer[3 + sizeof(int) + i * ((sizeof(int) + 1)) + sizeof(int)] = info->car_type;
            }
            break;
        }
        case CAR_SELECTION: {
            car_selection_packet_t *packet = (car_selection_packet_t*)p;
            *size = CAR_SELECTION_PACKET_SIZE;
            buffer = malloc(*size);
            buffer[0] = type;
            buffer[1] = (unsigned char)packet->car_type;
            break;
        }
        case UPDATE_ID: {
            update_client_id_packet_t *packet = (update_client_id_packet_t*)p;
            *size = 1 + sizeof(int);
            buffer = malloc(*size);
            buffer[0] = type;
            *(int*)(buffer + 1) = packet->id;
            break;
        }
        case MULTI_WINNER: {
            multi_winner_packet_t *packet = (multi_winner_packet_t*)p;
            *size = 1 + sizeof(int);
            buffer = malloc(*size);
            buffer[0] = type;
            *(int*)(buffer + 1) = packet->id;
            break;
        }
        case ALL_UPDATES: {
            all_car_body_packet_t *packet = (all_car_body_packet_t*)p;
            size_t block_size = sizeof(int) + 3 * sizeof(double);
            *size = 2 + packet->client_count * block_size;
            buffer = malloc(*size);
            buffer[0] = type;
            buffer[1] = (unsigned char)packet->client_count;
            for (size_t i = 0; i < list_size(packet->car_body_data); ++i) {
                car_body_data_t *data = (car_body_data_t*)list_get(packet->car_body_data, i);
                *(int*)(buffer + 2 + i * block_size) = data->client_id;
                *(double*)(buffer + 2 + i * block_size + sizeof(int)) = data->x;
                *(double*)(buffer + 2 + i * block_size + sizeof(int) + sizeof(double)) = data->y;
                *(double*)(buffer + 2 + i * block_size + sizeof(int) + 2 * sizeof(double)) = data->angle;
            }
            break;
        } case UPDATE_LAPS: {
            update_lap_counter_packet_t *packet = (update_lap_counter_packet_t*)p;
            *size = 1 + sizeof(int) * 2;
            buffer = malloc(*size);
            buffer[0] = type;
            *(int*)(buffer + 1) = packet->lap_count;
            *(int*)(buffer + 1 + sizeof(int)) = packet->lap_max;
            break;
        }
        default: break;
    }
    return buffer;
}

void packet_tests() {

    puts("CLIENT_INPUT TESTS"); {
        client_input_packet_t packet = (client_input_packet_t){
            CLIENT_INPUT, (input_data_t) { 1, 1, 1, 0, 0 }
        };
        uint64_t size;
        const unsigned char *buffer = write_packet(&packet, CLIENT_INPUT, &size);
        client_input_packet_t *reconstructed = (client_input_packet_t*)parse_packet(buffer, size);

        assert(reconstructed->type == packet.type);
        assert(reconstructed->data.left_held == packet.data.left_held);
        assert(reconstructed->data.down_held == packet.data.down_held);
        assert(reconstructed->data.space_held == packet.data.space_held);
        assert(reconstructed->data.up_held == packet.data.up_held);
        assert(reconstructed->data.right_held == packet.data.right_held);
        free(buffer);
        free(reconstructed);
    }
    

    puts("LOBBY_LIST TESTS"); {
        list_t *client_cars = list_init(3, NULL);
        client_lobby_info_t car0 = (client_lobby_info_t){ 0 };
        client_lobby_info_t car1 = (client_lobby_info_t){ 5 };
        client_lobby_info_t car2 = (client_lobby_info_t){ 3 };
        list_add(client_cars, &car0);
        list_add(client_cars, &car1);
        list_add(client_cars, &car2);
        lobby_list_packet_t  packet = (lobby_list_packet_t) {
            LOBBY_LIST, 3, client_cars
        };

        uint64_t size;
        const unsigned char *buffer = write_packet(&packet, LOBBY_LIST, &size);
        lobby_list_packet_t *reconstructed = (lobby_list_packet_t*)parse_packet(buffer, size);
        list_t *client_cars_r = reconstructed->clients;
        assert(reconstructed->type == packet.type);
        assert(reconstructed->client_count == packet.client_count);
        client_lobby_info_t *car0r = (client_lobby_info_t*)list_get(client_cars_r, 0);
        client_lobby_info_t *car1r = (client_lobby_info_t*)list_get(client_cars_r, 1);
        client_lobby_info_t *car2r = (client_lobby_info_t*)list_get(client_cars_r, 2);
        assert(car0r->client_id == car0.client_id);
        assert(car1r->client_id == car1.client_id);
        assert(car2r->client_id == car2.client_id);

        free(buffer);
        free(reconstructed);
        list_free(client_cars);
        list_free(client_cars_r);
    }
    
    puts("START_MULTI_GAME TESTS"); {
        list_t *car_info = list_init(3, free);
        client_car_info_t car0 = (client_car_info_t){ 0, 5 };
        client_car_info_t car1 = (client_car_info_t){ 5, 2 };
        client_car_info_t car2 = (client_car_info_t){ 3, 1 };
        list_add(car_info, &car0);
        list_add(car_info, &car1);
        list_add(car_info, &car2);
        start_multi_game_packet_t  packet = (start_multi_game_packet_t) {
            START_MULTI_GAME, 1, 3, 99, car_info
        };

        uint64_t size;
        const unsigned char *buffer = write_packet(&packet, START_MULTI_GAME, &size);
        start_multi_game_packet_t *reconstructed = (start_multi_game_packet_t*)parse_packet(buffer, size);
        list_t *client_cars_r = reconstructed->car_info;
        printf("%i | %i \n", reconstructed->type, packet.type);
        assert(reconstructed->type == packet.type);
        assert(reconstructed->client_count == packet.client_count);
        assert(reconstructed->local_client_id == packet.local_client_id);
        assert(reconstructed->map_type == packet.map_type);

        client_car_info_t *car0r = (client_car_info_t*)list_get(client_cars_r, 0);
        client_car_info_t *car1r = (client_car_info_t*)list_get(client_cars_r, 1);
        client_car_info_t *car2r = (client_car_info_t*)list_get(client_cars_r, 2);
        assert(car0r->client_id == car0.client_id);
        assert(car1r->client_id == car1.client_id);
        assert(car2r->client_id == car2.client_id);

        assert(car0r->car_type == car0.car_type);
        assert(car1r->car_type == car1.car_type);
        assert(car2r->car_type == car2.car_type);
    }

    puts("CAR_SELECTION TESTS"); {
        car_selection_packet_t packet = (car_selection_packet_t){
            CAR_SELECTION, 5
        };
        uint64_t size;
        const unsigned char *buffer = write_packet(&packet, CAR_SELECTION, &size);
        car_selection_packet_t *reconstructed = (car_selection_packet_t*)parse_packet(buffer, size);

        printf("%i | %i \n", reconstructed->type, packet.type);
        assert(reconstructed->type == packet.type);
        assert(reconstructed->car_type == packet.car_type);
    }

    puts("ALL_UPDATES"); {
        list_t *car_body_data = list_init(3, free);
        car_body_data_t car0 = (car_body_data_t){ 1, 1, 2, 3 };
        car_body_data_t car1 = (car_body_data_t){ 1, 5, 6, 7 };
        car_body_data_t car2 = (car_body_data_t){ 1, 2, 3, 4 };
        list_add(car_body_data, &car0);
        list_add(car_body_data, &car1);
        list_add(car_body_data, &car2);
        all_car_body_packet_t packet = (all_car_body_packet_t) {
            ALL_UPDATES, 3, car_body_data
        };

        uint64_t size;
        const unsigned char *buffer = write_packet(&packet, ALL_UPDATES, &size);
        all_car_body_packet_t *reconstructed = (all_car_body_packet_t*)parse_packet(buffer, size);
        list_t *client_cars_r = reconstructed->car_body_data;
        printf("%i | %i \n", reconstructed->type, packet.type);
        assert(reconstructed->type == packet.type);
        assert(reconstructed->client_count == packet.client_count);

        car_body_data_t *car0r = (car_body_data_t*)list_get(client_cars_r, 0);
        car_body_data_t *car1r = (car_body_data_t*)list_get(client_cars_r, 1);
        car_body_data_t *car2r = (car_body_data_t*)list_get(client_cars_r, 2);
        assert(car0r->client_id == car0.client_id);
        assert(car1r->client_id == car1.client_id);
        assert(car2r->client_id == car2.client_id);

        assert(isclose(car0r->x, car0.x));
        assert(isclose(car0r->y, car0.y));
        assert(isclose(car0r->angle, car0.angle));

        assert(isclose(car1r->x, car1.x));
        assert(isclose(car1r->y, car1.y));
        assert(isclose(car1r->angle, car1.angle));

        assert(isclose(car2r->x, car2.x));
        assert(isclose(car2r->y, car2.y));
        assert(isclose(car2r->angle, car2.angle));
    }

    puts("UPDATE_ID"); {
        update_client_id_packet_t packet = (update_client_id_packet_t){
            UPDATE_ID, 5
        };
        uint64_t size;
        const unsigned char *buffer = write_packet(&packet, UPDATE_ID, &size);
        update_client_id_packet_t *reconstructed = (update_client_id_packet_t*)parse_packet(buffer, size);

        printf("%i | %i \n", reconstructed->type, packet.type);
        assert(reconstructed->type == packet.type);
        assert(reconstructed->id == packet.id);
    }

    puts("UPDATE_LAPS"); {
        update_lap_counter_packet_t packet = (update_lap_counter_packet_t){
            UPDATE_LAPS, 5, 10
        };
        uint64_t size;
        const unsigned char *buffer = write_packet(&packet, UPDATE_LAPS, &size);
        update_lap_counter_packet_t *reconstructed = (update_lap_counter_packet_t*)parse_packet(buffer, size);

        printf("%i | %i \n", reconstructed->type, packet.type);
        assert(reconstructed->type == packet.type);
        assert(reconstructed->lap_max == packet.lap_max);
        assert(reconstructed->lap_count == packet.lap_count);

    }
}