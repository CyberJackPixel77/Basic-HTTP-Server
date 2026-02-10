#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <io.h>
#include "server.h"
#pragma comment(lib, "ws2_32.lib")


//MASTER
DWORD WINAPI MasterHandleClient(LPVOID client_socket) {
	//Master-thread [L1]
	SOCKET clientsocket = (SOCKET)client_socket;
	HTTP_REQUEST httpreq = { 0 };

	char client_message[SERV_MAX_BUFFER];
	char server_answer[SERV_MAX_BUFFER];

	//Functions (Workers: parce_HTTP_req, create_HTTP_answ, send_HTTP_answ) [L2]
	HTTP_PROTO_CODES res = handle_HTTP_req(&httpreq, client_message, clientsocket);
	BOOL val = create_HTTP_answer(&httpreq, server_answer, res);
	if (val != TRUE) return 1;
	send_HTTP_answer(&httpreq, clientsocket, server_answer, res);
	gracefull_shutdown(clientsocket);
	closesocket(clientsocket);
	return 0;
}

// ================= LAYER ZERO FUNCTIONS ==================
NET_STATUS initialize_net(SOCKET* OUT_server_socket) {
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		return WSASTARTUP_FAILED;
	}

	SOCKET server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == INVALID_SOCKET) {
		WSACleanup();
		return SERVER_SOCKET_INIT_FAILED;
	}

	struct sockaddr_in socket_address;
	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = INADDR_ANY;
	socket_address.sin_port = htons(8080);

	if (bind(server_socket, (struct sockaddr*)&socket_address, sizeof(socket_address)) == SOCKET_ERROR) {
		closesocket(server_socket);
		WSACleanup();
		return SERVER_SOCKET_BIND_FAILRED;
	}

	if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(server_socket);
		WSACleanup();
		return SERVER_SOCEKT_LISTEN_FAILED;
	}

	printf("Server is listening port 8080...\n");
	*OUT_server_socket = server_socket;
	return NET_STATUS_OK;
}
unsigned int run_accept_loop(SOCKET IN_server_socket) {
	while (1) {
		SOCKET client_socket = accept(IN_server_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) continue;
		else {
			//We've got socket
			set_socket_timeout(client_socket, 5);
			HANDLE client_thread;
			client_thread = CreateThread(NULL, 0, MasterHandleClient, (LPVOID)client_socket, 0, NULL);
			if (client_thread == NULL) continue;
			CloseHandle(client_thread);
		}
	}
}
void set_socket_timeout(SOCKET clientsocket, int seconds) {
	DWORD timeout = seconds * 1000;
	setsockopt(clientsocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
}
// ================= LAYER ONE FUNCTIONS ===================
//	Master-Thread
// ================= Layer TWO Functions ===================
HTTP_PROTO_CODES handle_HTTP_req(HTTP_REQUEST* ptr_to_HTTP_struct, char* client_message, SOCKET client_socket) {
	//Getting raw bytes [get_raw_bytes - L3]
	get_raw_bytes(client_message, client_socket);
	parce_HTTP_req(ptr_to_HTTP_struct, client_message);
	HTTP_PROTO_CODES res = validate_HTTP_request(ptr_to_HTTP_struct, client_message);
	if (res != OK) return res;
	return OK;

}
BOOL create_HTTP_answer(HTTP_REQUEST* ptr_to_HTTP_struct, char* server_answer_buffer, HTTP_PROTO_CODES code) {

	HTTP_METHOD method = get_HTTP_method(ptr_to_HTTP_struct);
	HTTP_ANSWER_DATA httpanswdata = { method, code };
	size_t len = create_basic_headers(ptr_to_HTTP_struct, server_answer_buffer, code);
	char* write_to_server_buff = server_answer_buffer + len;

	if (len >= SERV_MAX_BUFFER) {
		return FALSE;
	}

	size_t remaining_size_in_buff = SERV_MAX_BUFFER - len;

	BOOL stat = handle_common_errs(write_to_server_buff, code, remaining_size_in_buff);

	if (method == HTTP_OPTIONS) {
		handle_options_method(write_to_server_buff, ptr_to_HTTP_struct, code, remaining_size_in_buff);
		return TRUE;
	}

	int var = universal_handle_method(write_to_server_buff, ptr_to_HTTP_struct, code, method, remaining_size_in_buff);
	if (var == 1) return FALSE;

	return TRUE;
}
void send_HTTP_answer(HTTP_REQUEST* ptr_to_HTTP_struct, SOCKET clientsocket, char* server_buffer, HTTP_PROTO_CODES code) {
	int bytes_sent = send(clientsocket, server_buffer, (int)strlen(server_buffer), 0);
	if (code != OK) return;


	if (!(strcmp(ptr_to_HTTP_struct->method, "GET") == 0)) return;
	else {
		char buf[MAX_PATH];
		int r = is_content_present(ptr_to_HTTP_struct->path, 1, buf);
		if (r != 11) return;
		FILE* file = fopen(buf, "rb");
		if (file == NULL) return;

		//Its GET
		size_t bytes_read;

		while ((bytes_read = fread(server_buffer, 1, SERV_MAX_BUFFER, file)) > 0) {
			bytes_sent = send(clientsocket, server_buffer, (int)bytes_read, 0);
			
			if (bytes_sent <= 0) {
				break; //Something wrong not on server side - conn close (this includes 0 bytes and SOCKET_ERROR flag)
			}
		}
		fclose(file);
	}
	return;
}
// ================= Layer THREE Functions ===================
HTTP_PROTO_CODES validate_HTTP_request(HTTP_REQUEST* ptr_to_HTTP_structure, char* client_message) {
	//Checking 
	SAFE_BOOL res = is_proto_allowed(ptr_to_HTTP_structure);
	if (res != TRUE_STATEMENT) return VERSION_NOT_SUPPORTED;

	res = is_server_present_in_path(ptr_to_HTTP_structure);
	if (res != TRUE_STATEMENT) return BAD_REQUEST;

	res = checkhostswrap(client_message, ptr_to_HTTP_structure);
	if (res == FALSE_STATEMENT) return BAD_REQUEST;

	res = is_method_allowed(ptr_to_HTTP_structure);
	if (res == FALSE_STATEMENT) return METHOD_NOT_ALLOWED;

	res = inspect_HTTP_request(client_message, ptr_to_HTTP_structure);
	if (res != OK) return res;

	return OK;
}
void get_raw_bytes(char* client_mssg_buffer, SOCKET client_socket) {
	size_t bytes_recved = recv(client_socket, client_mssg_buffer, (SERV_MAX_BUFFER - 1), 0);
	if (bytes_recved > 0) {
		client_mssg_buffer[bytes_recved] = '\0';
	}
}
void handle_options_method(char* server_answer_buffer, HTTP_REQUEST* ptr_to_HTTP_struct, HTTP_PROTO_CODES code, size_t left_space) {
	if (code == VERSION_NOT_SUPPORTED) {
		strcpy_s(server_answer_buffer, left_space, PRESET_HTTP_VERSION_NOT_SUPPORTED);
		return;
	}

	if (strcmp(ptr_to_HTTP_struct->path, "*") == 0) {
		strcpy_s(server_answer_buffer, left_space, PRESET_HTTP_SERVER_OPTIONS_REQ);
		return;
	}
	else {
		switch (code) {
		case FORBIDDEN: strcpy_s(server_answer_buffer, left_space, PRESET_HTTP_FORBIDDEN_OPIONS_REQ); break;
		case NO_CONTENT: strcpy_s(server_answer_buffer, left_space, PRESET_HTTP_SERVER_OPTIONS_REQ); break;
		case NOT_FOUND: strcpy_s(server_answer_buffer, left_space, PRESET_HTTP_NOT_FOUND); break;
		case OK: strcpy_s(server_answer_buffer, left_space, PRESET_HTTP_SERVER_OPTIONS_REQ); break;
		//case VERSION_NOT_SUPPORTED: strcpy_s(server_answer_buffer, left_space, PRESET_HTTP_VERSION_NOT_SUPPORTED); break;
		}
		return;
	}
}
void parce_HTTP_req(HTTP_REQUEST* ptr_to_HTTP_struct, char* client_message) {
	memset(ptr_to_HTTP_struct, 0, sizeof(HTTP_REQUEST));
	sscanf(client_message, "%9s %255s %19s", ptr_to_HTTP_struct->method, ptr_to_HTTP_struct->path, ptr_to_HTTP_struct->proto);
}
void gracefull_shutdown(SOCKET clientsocket) {
	if (shutdown(clientsocket, SD_SEND) == -1) return;
	size_t bytes_received;
	char eocbuffer[500];

	while ((bytes_received = recv(clientsocket, eocbuffer, sizeof(eocbuffer), 0)) > 0);
}
size_t create_basic_headers(HTTP_REQUEST* ptr_to_HTTP_struct, char* server_answer_buffer, HTTP_PROTO_CODES code) {
	char wordcode[40];
	get_wordcode(code, wordcode);
	if (code == VERSION_NOT_SUPPORTED) {
		snprintf(server_answer_buffer, SERV_MAX_BUFFER, "HTTP/1.1 %d %s", code, wordcode);
	}
	else {
		snprintf(server_answer_buffer, SERV_MAX_BUFFER, "%s %d %s", ptr_to_HTTP_struct->proto, code, wordcode);
	}
	return strlen(server_answer_buffer);
}
BOOL handle_common_errs(char* server_answer_buffer, HTTP_PROTO_CODES code, size_t remaining_space) {
	if (code != BAD_REQUEST && code != METHOD_NOT_ALLOWED && code != INTERNAL_SERVER_ERROR && code != VERSION_NOT_SUPPORTED) return FALSE;
	switch (code) {
	case BAD_REQUEST:            strcpy_s(server_answer_buffer, remaining_space, PRESET_HTTP_BAD_REQUEST);             break;
	case METHOD_NOT_ALLOWED:     strcpy_s(server_answer_buffer, remaining_space, PRESET_HTTP_METHOD_NOT_ALLOWED);      break;
	case INTERNAL_SERVER_ERROR:  strcpy_s(server_answer_buffer, remaining_space, PRESET_HTTP_INTERNAL_SERVER_ERROR);   break;
	}
	return TRUE;
}
int universal_handle_method(char* server_answer_buffer, HTTP_REQUEST* ptr_to_HTTP_struct, HTTP_PROTO_CODES code, HTTP_METHOD method, size_t remaining_space) {
	switch (code) {
	case NOT_FOUND: strcpy_s(server_answer_buffer, remaining_space, PRESET_HTTP_NOT_FOUND); return 0; //short answer
	case FORBIDDEN: strcpy_s(server_answer_buffer, remaining_space, PRESET_HTTP_FORBIDDEN); return 0; //short answer
	case VERSION_NOT_SUPPORTED: strcpy_s(server_answer_buffer, remaining_space, PRESET_HTTP_VERSION_NOT_SUPPORTED); return 0;
	  case OK: {
		  BOOL stat = handle_successfull_answer(server_answer_buffer, ptr_to_HTTP_struct, method);
		  if (stat != TRUE) return 1; //Err occured
		  
		  if ((method == HTTP_GET) && (stat == TRUE)) return 2; //We need to send file
		  if ((method == HTTP_HEAD) && (stat == TRUE)) return 3; //We need to send headers
	  }
	}
	return 0;
}

BOOL is_request_successfull(HTTP_PROTO_CODES* code) {
	if (code != OK) return FALSE;
	else return TRUE;
}
// ================ Layer FOUR Functions =====================
SAFE_BOOL is_proto_allowed(HTTP_REQUEST* ptr_to_HTTP_struct) {
	return ((strcmp(ptr_to_HTTP_struct->proto, "HTTP/1.1") == 0) || (strcmp(ptr_to_HTTP_struct->proto, "HTTP/1.0") == 0)) ? TRUE_STATEMENT : FALSE_STATEMENT;
}
SAFE_BOOL is_server_present_in_path(HTTP_REQUEST* ptr_to_HTTP_struct) {
	char* PTO = strstr(ptr_to_HTTP_struct->path, "SERVER\\");
	if (PTO != NULL) return TRUE_STATEMENT;
	else {
		if ((strcmp(ptr_to_HTTP_struct->path, "*") == 0) && (strcmp(ptr_to_HTTP_struct->method, "OPTIONS") == 0)) return TRUE_STATEMENT;
		else return FALSE_STATEMENT;
	}
	//return (PTO == NULL) ? FALSE_STATEMENT : TRUE_STATEMENT;
}
SAFE_BOOL checkhostswrap(char* client_message, HTTP_REQUEST* ptr_to_HTTP_struct) {
	SAFE_BOOL res = checkhosts(client_message, ptr_to_HTTP_struct);
	if (res == FALSE_STATEMENT && (strcmp(ptr_to_HTTP_struct->proto, "HTTP/1.1") == 0)) return FALSE_STATEMENT;
	return TRUE_STATEMENT;
}
SAFE_BOOL is_method_allowed(HTTP_REQUEST* ptr_to_HTTP_struct) {
	if (!((strcmp(ptr_to_HTTP_struct->method, "GET") == 0) || (strcmp(ptr_to_HTTP_struct->method, "HEAD") == 0) || (strcmp(ptr_to_HTTP_struct->method, "OPTIONS") == 0))) return FALSE_STATEMENT;
	return TRUE_STATEMENT;
}
HTTP_PROTO_CODES inspect_HTTP_request(char* client_message, HTTP_REQUEST* ptr_to_HTTP_struct) {
	char path_buffer[256];
	copy_path_to_safe_buffer(path_buffer, ptr_to_HTTP_struct);
	if ((strstr(path_buffer, "..") != NULL) || (strstr(path_buffer, "\\\\") != NULL) || (strstr(path_buffer, "%00") != NULL)) return FORBIDDEN;
	get_user_info_cycle_wrapper(client_message, ptr_to_HTTP_struct);
	HTTP_METHOD method = get_HTTP_method(ptr_to_HTTP_struct);
	if ((method == HTTP_OPTIONS) && (strcmp(ptr_to_HTTP_struct->path, "*") == 0)) return OK;

	SAFE_BOOL is_present = is_content_present(ptr_to_HTTP_struct->path, 0, NULL);
	if (is_present != TRUE_STATEMENT) return NOT_FOUND;

	SAFE_BOOL allowed = is_content_allowed(path_buffer, ptr_to_HTTP_struct, method);
	if (allowed != TRUE_STATEMENT) return FORBIDDEN;
	else return OK;
}

BOOL handle_successfull_answer(char* server_answer_buffer, HTTP_REQUEST* ptr_to_HTTP_struct, HTTP_METHOD method) {
	//sprintf(server_answer_buffer, "%s 200 OK\r\nServer: ThunderNET v1.0\r\n", ptr_to_HTTP_struct->proto);
	BOOL stat = getting_headers_ready(method, ptr_to_HTTP_struct, server_answer_buffer);
	if (stat != TRUE) return FALSE;
	else return TRUE;
}
void get_wordcode(HTTP_PROTO_CODES code, char* wordcode) {
	switch (code) {
	case 200:
		strcpy_s(wordcode, 40, "OK\r\n");
		break;
	case 204:
		strcpy_s(wordcode, 40, "No Content\r\n");
		break;
	case 400:
		strcpy_s(wordcode, 40, "Bad Request\r\n");
		break;
	case 403:
		strcpy_s(wordcode, 40, "Forbidden\r\n");
		break;
	case 404:
		strcpy_s(wordcode, 40, "Not Found\r\n");
		break;
	case 405:
		strcpy_s(wordcode, 40, "Method Not Allowed\r\n");
		break;
	case 500:
		strcpy_s(wordcode, 40, "Internal Server Error\r\n");
		break;
	case 505:
		strcpy_s(wordcode, 40, "HTTP Version Not Supported\r\n");
		break;
	default:
		strcpy_s(wordcode, 40, "Unknown\r\n");
		break;
	}
	return;
}
// ================ Layer FIVE Functtions ====================
SAFE_BOOL checkhosts(char* http_request, HTTP_REQUEST* ptr_to_hhtprequest_struct) {
	if ((http_request == NULL) || (ptr_to_hhtprequest_struct == NULL)) return NULL_POINTER;

	//Checking if request includes "Hosts: " line or not
	char* PointerToHosts = strstr(http_request, "Host:");
	if (PointerToHosts == NULL) return FALSE_STATEMENT;
	else {
		char* ptr_to_domain = PointerToHosts + strlen("Host:"); //pointer to first symbol of domain
		while (*ptr_to_domain == ' ') ptr_to_domain++;

		char* Pointer_to_pathstructure = ptr_to_hhtprequest_struct->host;
		size_t index = 0;

		size_t max_host_len = sizeof(ptr_to_hhtprequest_struct->host) - 1;
		while (index < max_host_len) {
			//Reading domain until we'll see '\r\n'
			if (((*ptr_to_domain) != '\r') && ((*ptr_to_domain) != '\0') && ((*ptr_to_domain) != '\n') && ((*ptr_to_domain) != ' ')) {
				ptr_to_hhtprequest_struct->host[index] = (*ptr_to_domain);
				ptr_to_domain++;
				index++;
			}
			else {
				ptr_to_hhtprequest_struct->host[index] = '\0';
				break;
			}
		}
		return (index > 1) ? TRUE_STATEMENT : FALSE_STATEMENT;
	}
}
HTTP_METHOD get_HTTP_method(HTTP_REQUEST* ptr_to_HTTP_struct) {
	if (strcmp(ptr_to_HTTP_struct->method, "GET") == 0) return HTTP_GET;
	else if (strcmp(ptr_to_HTTP_struct->method, "HEAD") == 0) return HTTP_GET;
	else return HTTP_OPTIONS;
}
SAFE_BOOL is_content_present(char* path_to_file, int method, char* ptr_to_out_path) {
	//Method - 0 means (is content present), 1 - means (get filepath)
	size_t type_of_task;

	if (ptr_to_out_path == NULL && method == 0)  type_of_task = 0;
	else if (ptr_to_out_path != NULL && method != 0) type_of_task = 1;
	else return FALSE_STATEMENT;

	const char* server_path_sample = SERVER_WORKPATH;
	size_t spslen = strlen(server_path_sample);
	char path_buffer[256];
	char* PTO = strstr(path_to_file, "SERVER\\");
	if (PTO == NULL) return FALSE_STATEMENT;
	PTO = strchr(path_to_file, '\\');
	PTO++;
	size_t usrpath = strlen(PTO) + 1;

	size_t remaining_space = sizeof(path_buffer) - spslen - usrpath;

	//if (remaining_space < 0) return NULL_POINTER;  !!! EXCEPTION !!! could appear if line is too big

	memcpy(path_buffer, server_path_sample, spslen);
	memcpy(path_buffer + spslen, PTO, usrpath);

	if (type_of_task == 0) {
		//printf("Searching for: '%s'\n", path_buffer);
		//if (_access(path_buffer, 0) != 0) return FALSE_STATEMENT;
		//else return TRUE_STATEMENT;
		DWORD dwAttrib = GetFileAttributesA(path_buffer);

	    if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
			return TRUE_STATEMENT;
		}
		return FALSE_STATEMENT;
	}
	else {
		strcpy_s(ptr_to_out_path, MAX_PATH, path_buffer);
		return 11;
	}

}
SAFE_BOOL is_content_allowed(char* copy_path_buffer, HTTP_REQUEST* ptr_to_HTTP_struct, HTTP_METHOD method) {
	FILE_RULES rule[] = {
		{"File.ini", NULL, NULL},
		{"Chrome.txt", "Mozilla", NULL},
		{"Firefox.txt", "Chrome", NULL},
		{"File1.txt", NULL, "en-US"}
	};
	char* PTpath_to_file = strrchr(ptr_to_HTTP_struct->path, '\\');
	PTpath_to_file++;
	size_t val = get_index_from_list(PTpath_to_file, rule);
	if (val == 4) return TRUE_STATEMENT;
	SAFE_BOOL is_allowed = inspect_request_any_method(val, ptr_to_HTTP_struct);
	if (is_allowed == TRUE_STATEMENT) return TRUE_STATEMENT;
	else return FALSE_STATEMENT;
}
void copy_path_to_safe_buffer(char* safe_buffer, HTTP_REQUEST* ptr_to_HTTP_struct) {
	snprintf(safe_buffer, sizeof(safe_buffer), "%s", ptr_to_HTTP_struct->path);
}
void get_user_info_cycle_wrapper(char* client_message, HTTP_REQUEST* ptr_to_HTTP_struct) {
	for (int i = 1; i < 3; i++) {
		get_user_info(client_message, ptr_to_HTTP_struct, i);
	}
}
//------------------------------
void writefile_and_header(char* path_to_file, char* path_to_server_buffer) {
	FILE* file = fopen(path_to_file, "rb");
	if (file == NULL) abort();


}
CONTENT_TYPE get_content_type(char* winpath) {
	char* PTO = strrchr(winpath, '.');
	if (PTO == NULL) return applicationjs;

	if ((strcmp(PTO, ".jpg") == 0) || (strcmp(PTO, ".jpeg") == 0)) return imagejpeg;
	else if (strcmp(PTO, ".png") == 0) return imagejpeg;
	else if ((strcmp(PTO, ".css") == 0) || (strcmp(PTO, ".txt") == 0)) return textcss;
	else if ((strcmp(PTO, ".js") == 0) || (strcmp(PTO, ".exe") == 0)) return applicationjs;
	else if ((strcmp(PTO, ".jpeg") == 0) || (strcmp(PTO, ".jpg") == 0)) return imagejpeg;
	else if ((strcmp(PTO, ".txt") == 0) || (strcmp(PTO, ".html") == 0)) return htmltext;
	else return applicationjs;
}
BOOL getting_headers_ready(HTTP_METHOD method, HTTP_REQUEST* ptr_to_HTTP_struct, char* server_buffer) {
	char winpathbuf[MAX_PATH];
	BOOL is_pre = is_content_present(ptr_to_HTTP_struct->path, 1, winpathbuf);
	if (is_pre != 11) return FALSE;

	FILE* file = fopen(winpathbuf, "rb");
	if (!file) return FALSE;

	_fseeki64(file, 0, SEEK_END);
	long long size = _ftelli64(file);
	_fseeki64(file, 0, SEEK_SET);

	CONTENT_TYPE typeofcontent = get_content_type(winpathbuf);

	char* universal_ptr_to_contenttype = NULL;

	switch (typeofcontent) {
	case htmltext:      universal_ptr_to_contenttype = PRESET_MIME_texthtml; break;
	case textcss:       universal_ptr_to_contenttype = PRESET_MIME_textcss; break;
	case applicationjs: universal_ptr_to_contenttype = PRESET_MIME_appjs; break;
	case imagejpeg:     universal_ptr_to_contenttype = PRESET_MIME_imgjpg; break;
	case imagepng:      universal_ptr_to_contenttype = PRESET_MIME_imgpng; break;
	}
	if (universal_ptr_to_contenttype == NULL) { 
		fclose(file);
		return FALSE;
	}

	sprintf(server_buffer, "Content-Type: %s\r\nContent-Length: %lld\r\nConnection: close\r\n\r\n", universal_ptr_to_contenttype, size);
	fclose(file);
	return TRUE;
}
// ================ Layer SIX Functions ======================
SAFE_BOOL inspect_request_any_method(size_t index, HTTP_REQUEST* ptr_to_HTTP_struct) {
	switch (index) {
	case 0: return FALSE_STATEMENT;
	case 1: {
		if (strstr(ptr_to_HTTP_struct->UserAgent, "Google") == NULL) return FALSE_STATEMENT;
		else break;
	}
	case 2: {
		if (strstr(ptr_to_HTTP_struct->UserAgent, "Mozilla") == NULL) return FALSE_STATEMENT;
		else break;
	}
	case 3: {
		if ((strstr(ptr_to_HTTP_struct->AcceptLanguage, "en-US") != NULL) || (ptr_to_HTTP_struct->AcceptLanguage[0] == '\0')) return FALSE_STATEMENT;
		else break;
	}
	}
	return TRUE_STATEMENT;
}
size_t get_index_from_list(char* filename, FILE_RULES* rules) {
	//printf("[DEBUG] Comparing: '%s' with rules...\n", filename);
	size_t i;
	for (i = 0; i < 4; i++) {
		if (strcmp(filename, rules[i].filename) == 0) return i;
	}
	return 4;
}
void get_user_info(char* client_message, HTTP_REQUEST* ptr_to_HTTP_struct, int content_type) {
	char* universalptr = NULL;
	char* universalline = NULL;

	size_t universallen = 0;
	if (content_type == 1) {
		universalline = "User-Agent:";
		universalptr = ptr_to_HTTP_struct->UserAgent;
		universallen = strlen(universalline); //11
	}
	else if (content_type == 2) {
		universalline = "Accept-Language:";
		universalptr = ptr_to_HTTP_struct->AcceptLanguage;
		universallen = strlen(universalline); //16
	}
	else return;

	char* PTO = strstr(client_message, universalline);
	if (PTO == NULL) return;

	PTO = PTO + universallen;
	while (*PTO == ' ') PTO++;
	size_t index = 0;
	while (*PTO != '\r' && *PTO != '\n' && *PTO != '\0' && index < 99) {
		universalptr[index] = (*PTO);
		PTO++;
		index++;
	}
	universalptr[index] = '\0';
}
//============================================================


