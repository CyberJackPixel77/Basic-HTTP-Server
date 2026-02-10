#ifndef SERVER_H
#define SERVER_H
#define CHECKFUNC(function_call) do { \
	int status = (int)(function_call); \
	if (status != 4) { \
		printf("[MASTER] - [FAILED] with code %d", status); \
		return 1; \
	} \
} while(0)
#define SERV_MAX_BUFFER 4096
#define SERVER_WORKPATH "C:\\Server\\serverdata\\"
//----------------------- CONSTANTS & Quick answers ------------------------------
#define PRESET_HTTP_NO_CONTENT "Connection: close\r\n\r\n"
#define PRESET_HTTP_BAD_REQUEST "Connection: close\r\n\r\n"
#define PRESET_HTTP_FORBIDDEN "Connection: close\r\n\r\n"
#define PRESET_HTTP_NOT_FOUND "Connection: close\r\n\r\n"
#define PRESET_HTTP_METHOD_NOT_ALLOWED "Allow: GET, HEAD, OPTIONS\r\nConnection: close\r\n\r\n"
#define PRESET_HTTP_INTERNAL_SERVER_ERROR "Connection: close\r\n\r\n"
#define PRESET_HTTP_VERSION_NOT_SUPPORTED "Connection: close\r\n\r\n"
#define PRESET_HTTP_SERVER_OPTIONS_REQ "Allow: GET, HEAD, OPTIONS\r\nConnection: close\r\n\r\n"
#define PRESET_HTTP_FORBIDDEN_OPIONS_REQ "Allow: OPTIONS\r\nConnection: close\r\n\r\n"

//MIME
#define PRESET_MIME_texthtml "text/html\r\n"
#define PRESET_MIME_textcss "text/css\r\n"
#define PRESET_MIME_appjs "application/javascript\r\n"
#define PRESET_MIME_imgjpg "image/jpeg\r\n"
#define PRESET_MIME_imgpng "image/png\r\n"

//  ------------------  Public structures and enums  -------------------
typedef enum HTTP_CODES {
	OK = 200,
	NO_CONTENT = 204,
	BAD_REQUEST = 400,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	METHOD_NOT_ALLOWED = 405,
	INTERNAL_SERVER_ERROR = 500,
	VERSION_NOT_SUPPORTED = 505
} HTTP_PROTO_CODES;
typedef enum {
	HTTP_GET = 0,
	HTTP_HEAD,
	HTTP_OPTIONS
} HTTP_METHOD;
typedef enum safe_bool {
	NULL_POINTER = -1,
	FALSE_STATEMENT,
	TRUE_STATEMENT
} SAFE_BOOL;
typedef struct HTTP_REQUEST {
	char method[10];
	char path[256];
	char proto[20];
	char host[256];
	char UserAgent[100];
	char AcceptLanguage[100];
} HTTP_REQUEST;
typedef struct FILE_RULES {
	const char* filename;
	const char* forbidden_user_agent;
	const char* forbidden_language;
} FILE_RULES;
typedef struct {
	HTTP_METHOD method;
	HTTP_PROTO_CODES last_func_code;

}HTTP_ANSWER_DATA;
typedef enum content_type {
	htmltext = 0,
	textcss,
	applicationjs,
	imagejpeg,
	imagepng
}CONTENT_TYPE;
typedef enum {
	WSASTARTUP_FAILED = 0,
	SERVER_SOCKET_INIT_FAILED = 1,
	SERVER_SOCKET_BIND_FAILRED = 2,
	SERVER_SOCEKT_LISTEN_FAILED = 3,
	NET_STATUS_OK
} NET_STATUS;

// ---------------------- SERVER ABSTRACTION LAYER STRUCTURE -----------------------


// ================   Layer 0 - Main-Func =====================
NET_STATUS initialize_net(SOCKET* OUT_server_socket);
unsigned int run_accept_loop(SOCKET IN_server_socket);
void set_socket_timeout(SOCKET clientsocket, int seconds);
// ================   Layer 1 - Master-Thread   ===============
// ===================== Layer 2 Functions ====================
HTTP_PROTO_CODES handle_HTTP_req(HTTP_REQUEST* ptr_to_HTTP_struct, char* client_message, SOCKET client_socket);
BOOL create_HTTP_answer(HTTP_REQUEST* ptr_to_HTTP_struct, char* server_answer_buffer, HTTP_PROTO_CODES code);
void send_HTTP_answer(HTTP_REQUEST* ptr_to_HTTP_struct, SOCKET clientsocket, char* server_buffer, HTTP_PROTO_CODES code);
// ===================== Layer 3 Functions ====================
HTTP_PROTO_CODES validate_HTTP_request(HTTP_REQUEST* ptr_to_HTTP_structure, char* client_message);
BOOL handle_common_errs(char* server_answer_buffer, HTTP_PROTO_CODES code, size_t remaining_space);
int universal_handle_method(char* server_answer_buffer, HTTP_REQUEST* ptr_to_HTTP_struct, HTTP_PROTO_CODES code, HTTP_METHOD method, size_t remaining_space);
BOOL is_request_successfull(HTTP_PROTO_CODES* code);
void get_raw_bytes(char* client_mssg_buffer, SOCKET client_socket);
void handle_options_method(char* server_answer_buffer, HTTP_REQUEST* ptr_to_HTTP_struct, HTTP_PROTO_CODES code, size_t left_space);
void parce_HTTP_req(HTTP_REQUEST* ptr_to_HTTP_struct, char* client_message);
void gracefull_shutdown(SOCKET clientsocket);
size_t create_basic_headers(HTTP_REQUEST* ptr_to_HTTP_struct, char* server_answer_buffer, HTTP_PROTO_CODES code);
// ===================== Layer 4 Functions =====================
SAFE_BOOL is_proto_allowed(HTTP_REQUEST* ptr_to_HTTP_struct);
SAFE_BOOL is_server_present_in_path(HTTP_REQUEST* ptr_to_HTTP_struct);
SAFE_BOOL checkhostswrap(char* client_message, HTTP_REQUEST* ptr_to_HTTP_struct);
SAFE_BOOL is_method_allowed(HTTP_REQUEST* ptr_to_HTTP_struct);
HTTP_PROTO_CODES inspect_HTTP_request(char* client_message, HTTP_REQUEST* ptr_to_HTTP_struct);
BOOL handle_successfull_answer(char* server_answer_buffer, HTTP_REQUEST* ptr_to_HTTP_struct, HTTP_METHOD method);
void get_wordcode(HTTP_PROTO_CODES code, char* wordcode);
// ===================== Layer 5 Functions ======================
SAFE_BOOL checkhosts(char* http_request, HTTP_REQUEST* ptr_to_hhtprequest_struct);
HTTP_METHOD get_HTTP_method(HTTP_REQUEST* ptr_to_HTTP_struct);
SAFE_BOOL is_content_present(char* path_to_file, int method, char* ptr_to_out_path);
SAFE_BOOL is_content_allowed(char* copy_path_buffer, HTTP_REQUEST* ptr_to_HTTP_struct, HTTP_METHOD method);

void writefile_and_header(char* path_to_file, char* path_to_server_buffer);
CONTENT_TYPE get_content_type(char* winpath);
BOOL getting_headers_ready(HTTP_METHOD method, HTTP_REQUEST* ptr_to_HTTP_struct, char* server_buffer);
void copy_path_to_safe_buffer(char* safe_buffer, HTTP_REQUEST* ptr_to_HTTP_struct);
void get_user_info_cycle_wrapper(char* client_message, HTTP_REQUEST* ptr_to_HTTP_struct);
// ===================== Layer 6 Functions ======================
SAFE_BOOL inspect_request_any_method(size_t index, HTTP_REQUEST* ptr_to_HTTP_struct);
size_t get_index_from_list(char* filename, FILE_RULES* rules);
void get_user_info(char* client_message, HTTP_REQUEST* ptr_to_HTTP_struct, int content_type);
// ==============================================================





#endif
