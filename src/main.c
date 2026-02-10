#include <stdio.h>
#include <WinSock2.h>
#include "server.h"
#pragma comment(lib, "ws2_32.lib")
int main() {
	int return_code = 0;

	SOCKET server_sock = INVALID_SOCKET;


	//L0 FUNC
	CHECKFUNC(initialize_net(&server_sock));
	CHECKFUNC(run_accept_loop(server_sock));
}
