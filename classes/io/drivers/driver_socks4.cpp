 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/io/drivers/driver_socks4.h>

#if defined __WIN32__
#	include <windows.h>
#else
#	include <unistd.h>
#	include <arpa/inet.h>
#	include <sys/socket.h>
#	include <netdb.h>
#	include <netinet/in.h>
#endif

S::IO::DriverSOCKS4::DriverSOCKS4(const String &proxy, Int socksPort, const String &hostName, Int port) : Driver()
{
	closeStream = False;

	stream	    = -1;
	size	    = -1;

	if (hostName == NIL || hostName.Length() > 255) { lastError = IO_ERROR_BADPARAM; return; }

	/* Open TCP/IP socket.
	 */
	stream = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (stream == (unsigned) (~0)) { lastError = IO_ERROR_UNEXPECTED; return; }

	closeStream = True;

	/* Get proxy and server hostname.
	 */
	hostent		*host = gethostbyname(proxy);

	if (host == NIL) { Close(); lastError = IO_ERROR_UNEXPECTED; return; }

	/* Connect to proxy.
	 */
	sockaddr_in	 saddr;

	saddr.sin_family = AF_INET;
	saddr.sin_addr	 = *((in_addr *) *host->h_addr_list);
	saddr.sin_port	 = htons((short) socksPort);

	memset(&saddr.sin_zero, 0, sizeof(saddr.sin_zero));

	if (connect(stream, (sockaddr *) &saddr, sizeof(struct sockaddr)) == -1) { Close(); lastError = IO_ERROR_UNEXPECTED; return; }

	/* Send connect request.
	 */
	hostent		*server_hostent = gethostbyname(hostName);

	if (server_hostent != NULL)
	{
		unsigned char	*socksdata = new unsigned char [9];

		socksdata[0] = 4;
		socksdata[1] = 1;
		socksdata[2] = htons((short) port) % 256;
		socksdata[3] = htons((short) port) / 256;
		socksdata[4] = server_hostent->h_addr_list[0][0];
		socksdata[5] = server_hostent->h_addr_list[0][1];
		socksdata[6] = server_hostent->h_addr_list[0][2];
		socksdata[7] = server_hostent->h_addr_list[0][3];
		socksdata[8] = 0;

		if (send(stream, (char *) socksdata, 9, 0) < 9) { delete [] socksdata; Close(); lastError = IO_ERROR_UNEXPECTED; return; }

		delete [] socksdata;
	}
	else if (inet_addr(hostName) != INADDR_NONE)
	{
		unsigned char	*socksdata = new unsigned char [9];

		socksdata[0] = 4;
		socksdata[1] = 1;
		socksdata[2] = htons((short) port) % 256;
		socksdata[3] = htons((short) port) / 256;
		socksdata[4] = IOGetByte(inet_addr(hostName), 0);
		socksdata[5] = IOGetByte(inet_addr(hostName), 1);
		socksdata[6] = IOGetByte(inet_addr(hostName), 2);
		socksdata[7] = IOGetByte(inet_addr(hostName), 3);
		socksdata[8] = 0;

		if (send(stream, (char *) socksdata, 9, 0) < 9) { delete [] socksdata; Close(); lastError = IO_ERROR_UNEXPECTED; return; }

		delete [] socksdata;
	}
	else
	{
		Int	 hostNameLen = strlen(hostName);

		unsigned char	*socksdata = new unsigned char [10 + hostNameLen];

		socksdata[0] = 4;
		socksdata[1] = 1;
		socksdata[2] = htons((short) port) % 256;
		socksdata[3] = htons((short) port) / 256;
		socksdata[4] = 0;
		socksdata[5] = 0;
		socksdata[6] = 0;
		socksdata[7] = 1;
		socksdata[8] = 0;

		for (Int i = 0; i < hostNameLen; i++) socksdata[9 + i] = hostName[i];

		socksdata[9 + hostNameLen] = 0;

		if (send(stream, (char *) socksdata, 10 + hostNameLen, 0) < 10 + hostNameLen) { delete [] socksdata; Close(); lastError = IO_ERROR_UNEXPECTED; return; }

		delete [] socksdata;
	}

	/* Receive answer.
	 */
	unsigned char	*socksdata = new unsigned char [8];
	int		 recbytes  = 0;

	while (recbytes != 8)
	{
		int	 bytes = recv(stream, (char *) socksdata + recbytes, 8 - recbytes, 0);

		if (bytes <= 0) { delete [] socksdata; Close(); lastError = IO_ERROR_UNEXPECTED; return; }

		recbytes += bytes;
	}

	/* Check if connect attempt was successful.
	 */
	if (socksdata[1] != 90) { delete [] socksdata; Close(); lastError = IO_ERROR_UNEXPECTED; return; }

	delete [] socksdata;
}

S::IO::DriverSOCKS4::~DriverSOCKS4()
{
	Close();
}

S::Int S::IO::DriverSOCKS4::ReadData(UnsignedByte *data, Int dataSize)
{
	if (dataSize <= 0) return 0;

	int	 bytes = recv(stream, (char *) data, dataSize, 0);

	if (bytes <= 0) return -1;
	else		return bytes;
}

S::Int S::IO::DriverSOCKS4::WriteData(const UnsignedByte *data, Int dataSize)
{
	if (dataSize <= 0) return 0;

	return send(stream, (const char *) data, dataSize, 0);
}

S::Bool S::IO::DriverSOCKS4::Close()
{
#if defined __WIN32__
	if (!closeStream || closesocket(stream) != 0) return False;
#else
	if (!closeStream || close(stream)	!= 0) return False;
#endif

	closeStream = False;

	return True;
}
