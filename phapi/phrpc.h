/*
  The phrpc module implements simple RPC interface to phApi server
  application based on eXosip stack
  Copyright (C) 2004  Vadim Lebedev  vadim@mbdsys.com
  
  
  This library is free software; you can redistribute it and/or
  modify it under any terms you wish provided this copyright message is included.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

*/
#ifndef _PHRPC_H_
#define _PHRPC_H_ 1

/**
 *  the port on which phApi server expects requests
 */
#define PH_SERVER_PORT  20000

/**
 * the port to which phApi server send callbacks
 */
#define PH_CALLBACK_PORT  20001


extern unsigned short phServerPort;
extern unsigned short phCallBackPort;

#endif
