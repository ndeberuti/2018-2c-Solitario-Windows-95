#include "servidor.h"

uint32_t build_server(uint32_t port, t_log *log_file) {
	uint32_t listener; // descriptor de socket a la escucha
	uint32_t yes=1; // para setsockopt() SO_REUSEADDR, más abajo
	struct sockaddr_in myaddr; // dirección del servidor

	// obtener socket a la escucha
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log_error(log_file, "socket");
		exit(EXIT_FAILURE);
	}

	// obviar el mensaje "address already in use" (la dirección ya se está usando)
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		log_error(log_file, "setsockopt");
		exit(EXIT_FAILURE);
	}

	// enlazar
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(port);
	memset(&(myaddr.sin_zero), '\0', 8);

	if (bind(listener, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
		log_error(log_file, "bind");
		exit(EXIT_FAILURE);
	}

	// escuchar
	if (listen(listener, 50) == -1) {
		log_error(log_file, "listen");
		exit(EXIT_FAILURE);
	}

	return listener;
}

uint32_t connect_server(char *dest_ip, uint32_t port, uint32_t handshake, t_log *log_file) {
	uint32_t sockfd;
	struct sockaddr_in their_addr; // información de la dirección de destino

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log_error(log_file, "socket");
		return 0;
	}

	their_addr.sin_family = AF_INET; // Ordenación de bytes de la máquina
	their_addr.sin_port = htons(port); // short, Ordenación de bytes de la red
	their_addr.sin_addr.s_addr = inet_addr(dest_ip);
	memset(&(their_addr.sin_zero), '\0', 8); // poner a cero el resto de la estructura

	if (connect(sockfd, (struct sockaddr *) &their_addr, sizeof(struct sockaddr)) == -1) {
		log_error(log_file, "connect");
		return 0;
	}

	if (handshake >= 0 && send_int(sockfd, handshake) == -1) {
		log_error(log_file, "handshake");
		return 0;
	}

	return sockfd;
}

uint32_t receive_int(uint32_t socket, uint32_t *buf) {
	return recv(socket, buf, sizeof(uint32_t), 0);
}

uint32_t receive_string(uint32_t socket, char **buf) {
	uint32_t nbytes;
	uint32_t size_of_buf;

	if ((nbytes = receive_int(socket, &size_of_buf)) <= 0)
		return nbytes;

	if ((*buf = malloc(sizeof(char) * (size_of_buf + 1))) == NULL)
		return -1;

	return recv(socket, *buf, size_of_buf, 0);
}

uint32_t send_string(uint32_t socket, char *buf) {
	uint32_t nbytes;
	uint32_t size_of_buf = strlen(buf) + 1;

	if ((nbytes = send_int(socket, size_of_buf)) == -1)
		return nbytes;

	return send(socket, buf, size_of_buf, 0);
}

uint32_t send_int(uint32_t socket, uint32_t buf) {
	return send(socket, &buf, sizeof(uint32_t), 0);
}
