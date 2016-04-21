#include "robot.h"

//Tests if str starts with test_str.
//Return 0 if str starts with test_str. -1 otherwise.
static int strstartsstr(struct mg_str *str, char* test_str){
	size_t test_str_len = strlen(test_str);
	if (test_str_len > str->len)
		return -1;

	int i = 0;
	for(; i < test_str_len; i++){
		if(str->p[i] != test_str[i])
			return -1;
	}

	return 0;
}

//Reads a file into memory.
static long read_file(char* file_name, char** file_data){
	FILE *f = fopen(file_name, "rb");

	if(f == NULL)
		return -1;

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	*file_data = string;
	return fsize;
}

//Loads the specified files into a linked list "html_pages"
static void load_files(char* files[], int num_files){
	struct file_node *tmp_ptr = NULL;

	int i = 0;
	for(; i < num_files; i++){
		char* file_string = 0;
		long file_size = read_file(files[i], &file_string);

		if(file_size < 1 || file_string == NULL)
			continue;

		if(i == 0){
			if(html_pages == NULL){
				html_pages = malloc(sizeof(struct file_node));
				tmp_ptr = html_pages;
			}else{
				for(tmp_ptr = html_pages; tmp_ptr->next != NULL; tmp_ptr = tmp_ptr->next);
				tmp_ptr->next = malloc(sizeof(struct file_node));
				tmp_ptr = tmp_ptr->next;
			}
		}else{
			tmp_ptr->next = malloc(sizeof(struct file_node));
			tmp_ptr = tmp_ptr->next;
		}

		tmp_ptr->file_name = files[i];
		tmp_ptr->file_data = file_string;
		tmp_ptr->file_size = file_size;	
		tmp_ptr->next = NULL;
	}
}

// Fills the file_data pointer with the data from the specified file.
// The file must be loaded first with load_files()
static long get_file(char* file_name, char** file_data){
	struct file_node *tmp_ptr = html_pages;

	for(; tmp_ptr != NULL; tmp_ptr = tmp_ptr->next){
		if(strcmp(tmp_ptr->file_name, file_name) == 0){
			*file_data = tmp_ptr->file_data;
			return tmp_ptr->file_size;
		}
	}

	return -1;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
	switch (ev) {
		case MG_EV_HTTP_REQUEST:
		{
			struct http_message* hm = (struct http_message *) ev_data;
			struct mg_str uri = hm->uri;
			struct mg_str method = hm->method;
			struct mg_str body = hm->body;

			if(mg_vcmp(&method, "GET") == 0){
				if(mg_vcmp(&uri, "/") == 0){
					char* response = 0;
					int response_length = get_file("private/robot_control.html", &response);

					if(response == NULL || response_length < 0){
						mg_printf(nc, "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-length: 44\r\n\r\n<html><body>Page not found. :(</body></html>");
						mg_send_http_chunk(nc, "", 0);
						break;
					}

					mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nContent-length: %i\r\n\r\n", response_length);
					mg_printf(nc, "%s", response);

					mg_send_http_chunk(nc, "", 0);
					break;
				}else if(strstartsstr(&uri, "/public/") == 0){
					int i = 0;
					for(; i < uri.len; i++){
						if(uri.p[i] == ' ')
							break;
					}

					char* file_path = malloc(sizeof(char) * (i + 1));
					memcpy(file_path, uri.p, i);
					file_path[i] = '\0';
					file_path++;

					char* response = 0;
					int response_length = get_file(file_path, &response);

					if(response_length < 0 || response == NULL){
						char* temp[1] = {file_path};
						load_files(temp, 1);
						response_length = get_file(file_path, &response);
						if(response_length < 0 || response == NULL){
							mg_printf(nc, "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-length: 44\r\n\r\n<html><body>File not found. :(</body></html>");
							mg_send_http_chunk(nc, "", 0);
							break;
						}
					}

					mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nContent-length: %i\r\n\r\n", response_length);
					mg_printf(nc, "%s", response);
					mg_send_http_chunk(nc, "", 0);

					free(--file_path);
					break;
				}else{
					mg_printf(nc, "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-length: 44\r\n\r\n<html><body>Page not found. :(</body></html>");
					mg_send_http_chunk(nc, "", 0);
					break;
				}
			}else if(mg_vcmp(&method, "POST") == 0){
				if(mg_vcmp(&uri, "/control_robot") == 0){
					memcpy(robot_commands, body.p, MIN(body.len, 512));

					mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nContent-length: 7\r\n\r\nSuccess");
					mg_send_http_chunk(nc, "", 0);
					break;
				}
			}

			mg_printf(nc, "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-length: 41\r\n\r\n<html><body>Unknown Request</body></html>");
			mg_send_http_chunk(nc, "", 0);			
			break;
		}

		default:
		{
			break;
		}
  }
}

static void * robot_thread(void* min_loop_time){
	long loop_time = (long) min_loop_time;

	if(start_pwm(LEFT_MOTOR, 7.5F, 50, 0) == 0 || start_pwm(RIGHT_MOTOR, 7.5F, 50, 0) == 0){
		fprintf(stderr, "Failed to initiliaze motors.\n");
	}

	while(thread_running){
		double now = cs_time();

		unsigned char left_key = robot_commands[0], up_key = robot_commands[1], right_key = robot_commands[2], down_key = robot_commands[3];
		float speed = ((float) robot_commands[4]) / 100.0F;

		if(up_key){
			if(left_key){
				set_duty_cycle_pwm(LEFT_MOTOR, 7.5F);
				set_duty_cycle_pwm(RIGHT_MOTOR, 7.5F + speed);
			}else if(right_key){
				set_duty_cycle_pwm(LEFT_MOTOR, 7.5F + speed);
				set_duty_cycle_pwm(RIGHT_MOTOR, 7.5F);
			}else{
				set_duty_cycle_pwm(LEFT_MOTOR, 7.5F + speed);
				set_duty_cycle_pwm(RIGHT_MOTOR, 7.5F + speed);
			}
		}else if(down_key){
			if(left_key){
				set_duty_cycle_pwm(LEFT_MOTOR, 7.5F - speed);
				set_duty_cycle_pwm(RIGHT_MOTOR, 7.5F);
			}else if(right_key){
				set_duty_cycle_pwm(LEFT_MOTOR, 7.5F);
				set_duty_cycle_pwm(RIGHT_MOTOR, 7.5F - speed);
			}else{
				set_duty_cycle_pwm(LEFT_MOTOR, 7.5F - speed);
				set_duty_cycle_pwm(RIGHT_MOTOR, 7.5F - speed);
			}
		}else if(right_key){
			set_duty_cycle_pwm(LEFT_MOTOR, 7.5F + speed);
			set_duty_cycle_pwm(RIGHT_MOTOR, 7.5F - speed);
		}else if(left_key){
			set_duty_cycle_pwm(LEFT_MOTOR, 7.5F - speed);
			set_duty_cycle_pwm(RIGHT_MOTOR, 7.5F + speed);
		}else{
			set_duty_cycle_pwm(LEFT_MOTOR, 7.5F);
			set_duty_cycle_pwm(RIGHT_MOTOR, 7.5F);
		}

		double elapsed = cs_time() - now;
		if(elapsed < loop_time)
			usleep((loop_time - elapsed) * 1000);
	}

	return NULL;
}

int main(int argc, char ** argv){
	load_files(private_files, sizeof private_files/sizeof *private_files);

	pthread_t thread_id;
	int ret = pthread_create(&thread_id, NULL, robot_thread, (void *) 25);

	if(ret < 0){
		fprintf(stderr, "Couldn't start robot thread.\n");
		return 1;
	}

	struct mg_mgr mgr;
	struct mg_connection *nc;

	mg_mgr_init(&mgr, NULL);
	nc = mg_bind(&mgr, SERVER_PORT, ev_handler);

	if(nc == NULL){
		fprintf(stderr, "Couldn't bind to port: " SERVER_PORT "\n");
		return 1;
	}

	mg_set_protocol_http_websocket(nc);

	for(;;){
		mg_mgr_poll(&mgr, 1000);
	}

	mg_mgr_free(&mgr);

	thread_running = 0;
	pthread_join(thread_id, NULL);
	return 0;
}