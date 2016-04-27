#include "robot.h"

static void get_mime_type(char* file_name, char* buffer, int len){
	char *file_ext = strrchr(file_name, '.');

    if(!file_ext || file_ext == file_name) return;

    file_ext = file_ext + 1;

    int i = 0;
    for(; i < sizeof(pairs) / sizeof(mime_pairs); i++){
    	if(strcmp(pairs[i][0], file_ext) == 0){
    		int pairs_length = strlen(pairs[i][1]);

    		if(len < 17 + pairs_length){
    			return;
    		}

    		memcpy(buffer, "Content-Type: ", 14);
    		memcpy(buffer + 14, pairs[i][1], pairs_length);
    		memcpy(buffer + 14 + pairs_length, "\r\n", 2);
    		buffer[16 + pairs_length] = '\0';

    		return;
    	}
    }
}

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

static int is_file(char* path){
	struct stat s;
	if(stat(path, &s) == 0){
	    if(s.st_mode & S_IFREG){
	    	return 1;
	    }
	}

	return 0;
}

//Reads a file into memory.
static long read_file(char* file_name, char** file_data){
	if(!is_file(file_name))
		return -1;

	FILE *f = fopen(file_name, "rb");

	if(f == NULL)
		return -1;

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *string = malloc(fsize + 1);
	if(string == NULL) return -1;

	fread(string, fsize, 1, f);
	fclose(f);

	*file_data = string;
	return fsize;
}

//Loads the specified file into a linked list "html_pages"
static void load_file(char* file){
	char* file_string = 0;
	long file_size = read_file(file, &file_string);

	if(file_size < 1 || file_string == NULL)
		return;

	struct file_node *new_node = malloc(sizeof(struct file_node));
	if(new_node == NULL) return;

	new_node->file_name = file;
	new_node->file_data = file_string;
	new_node->file_size = file_size;	
	new_node->next = NULL;

	if(html_pages == NULL){
		html_pages = new_node;
	}else{
		struct file_node *tmp_ptr = html_pages;
		for(;tmp_ptr->next != NULL; tmp_ptr = tmp_ptr->next);
		tmp_ptr->next = new_node;
	}
}

// Fills the file_data pointer with the data from the specified file.
// The file must be loaded first with load_files()
static long get_file(char* file_path, char** file_data){
	struct file_node *tmp_ptr = html_pages;

	for(; tmp_ptr != NULL; tmp_ptr = tmp_ptr->next){
		if(strcmp(tmp_ptr->file_name, file_path) == 0){
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

					//The following lines make sure the filepath doesn't have a ".." in it as that could be a vulnerability that gives access to all files.
					//For example someone could pass the path public/../../../../passwords.txt and it would push them up several directories and attempt to access the file passwords.txt in that directory.
					//I mean I don't think it matters that much that there is a vulnerability in our code because of the limited scope of our application
					//But like it felt necessary to fix.
					int flag = 0;
					int x = 0;
					for(; x < i; x++){
						if(file_path[x] == '.' && file_path[x + 1] == '.'){
							flag = 1;
							break;
						}
					}

					if(flag){
						mg_printf(nc, "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-length: 53\r\n\r\n<html><body>File path can't be accessed</body></html>");
						mg_send_http_chunk(nc, "", 0);
						break;
					}


					char* response = 0;
					int response_length = get_file(file_path, &response);

					if(response_length < 0 || response == NULL){
						load_file(file_path);
						response = 0;
						response_length = get_file(file_path, &response);
						if(response_length < 0 || response == NULL){
							mg_printf(nc, "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\nContent-length: 44\r\n\r\n<html><body>File not found. :(</body></html>");
							mg_send_http_chunk(nc, "", 0);
							break;
						}
					}

					char buffer[1024];
					buffer[0] = '\0';
					get_mime_type(file_path, buffer, 1024);

					mg_printf(nc, "HTTP/1.1 200 OK\r\n%sContent-length: %i\r\n\r\n", buffer, response_length);
					mg_send(nc, (void *) response, response_length);
					mg_send_http_chunk(nc, "", 0);

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

static void * non_autonomous_robot_thread(void* min_loop_time){
	long loop_time = (long) min_loop_time;

	if(start_pwm(LEFT_MOTOR, 7.5F, 50, 0) == 0 || start_pwm(RIGHT_MOTOR, 7.5F, 50, 0) == 0){
		fprintf(stderr, "Failed to initialize pins.\n");
	}else{
		printf("Initialized pins properly.\nRobot thread running.\n");
	}

	while(thread_running){
		double now = cs_time();

		unsigned char left_key = robot_commands[0], up_key = robot_commands[1], right_key = robot_commands[2], down_key = robot_commands[3];
		float speed = PWM_MOTOR_DUTY_CYCLE_RANGE * (((float) robot_commands[4]) / 100.0F);

		if(up_key){
			if(left_key){
				set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER);
				set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + speed);
			}else if(right_key){
				set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + speed);
				set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER);
			}else{
				set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + speed);
				set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + speed);
			}
		}else if(down_key){
			if(left_key){
				set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER - speed);
				set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER);
			}else if(right_key){
				set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER);
				set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER - speed);
			}else{
				set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER - speed);
				set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER - speed);
			}
		}else if(right_key){
			set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + speed);
			set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER - speed);
		}else if(left_key){
			set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER - speed);
			set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + speed);
		}else{
			set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER);
			set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER);
		}

		double elapsed = cs_time() - now;
		if(elapsed < loop_time)
			usleep((loop_time - elapsed) * 1000);
	}

	return NULL;
}

static void * autonomous_robot_thread(){
	if(start_pwm(LEFT_MOTOR, 7.5F, 50, 0) == 0 || start_pwm(RIGHT_MOTOR, 7.5F, 50, 0) == 0){
		fprintf(stderr, "Failed to initialize pins.\n");
	}else{
		printf("Initialized pins properly.\nRobot thread running.\n");
	}

	setup_channel_gpio(DISTANCE_SENSOR, INPUT, 0);
	set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + PWM_MOTOR_DUTY_CYCLE_RANGE);
	set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + PWM_MOTOR_DUTY_CYCLE_RANGE);

	while(1){
		unsigned int value = 0;
		input_gpio(DISTANCE_SENSOR, &value);
		if(value){
			set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER);
			set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER);
		}else{
			set_duty_cycle_pwm(LEFT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + PWM_MOTOR_DUTY_CYCLE_RANGE);
			set_duty_cycle_pwm(RIGHT_MOTOR, PWM_MOTOR_DUTY_CYCLE_CENTER + PWM_MOTOR_DUTY_CYCLE_RANGE);
		}
	}

	return NULL;
}

int main(int argc, char ** argv){
	#ifndef AUTONOMOUS_MODE

	int i = 0;
	for(; i < (sizeof private_files/sizeof *private_files); i++){
		load_file(private_files[i]);
	}

	pthread_t thread_id;
	int ret = pthread_create(&thread_id, NULL, non_autonomous_robot_thread, (void *) 25);

	if(ret < 0){
		fprintf(stderr, "Couldn't start robot thread.\n");
		return 1;
	}

	struct mg_mgr mgr;
	struct mg_connection *nc;

	mg_mgr_init(&mgr, NULL);
	nc = mg_bind(&mgr, SERVER_ADDRESS ":" SERVER_PORT, ev_handler);

	if(nc == NULL){
		fprintf(stderr, "Couldn't bind to address: " SERVER_ADDRESS ":" SERVER_PORT "\n");
		return 1;
	}

	mg_set_protocol_http_websocket(nc);
	printf("Webserver running on port: " SERVER_ADDRESS ":" SERVER_PORT "\n");

	for(;;){
		mg_mgr_poll(&mgr, 1000);
	}

	mg_mgr_free(&mgr);

	thread_running = 0;
	pthread_join(thread_id, NULL);
	return 0;

	#else

	autonomous_robot_thread();

	#endif
}