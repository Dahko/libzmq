/*
    Copyright (c) 2007-2014 Contributors as noted in the AUTHORS file

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "testutil.hpp"


void test_stream_2_stream(void* ctx_){
    void *rbind, *rconn1;
    int ret;
    char buff[256];
    char msg[] = "hi 1";
    const char *bindip = "tcp://127.0.0.1:5556";
    int zero = 0;

    //  Set up listener STREAM.
    rbind = zmq_socket (ctx_, ZMQ_STREAM);
    assert (rbind);
    ret = zmq_setsockopt (rbind, ZMQ_LINGER, &zero, sizeof (zero));
    assert (0 == ret);
    ret = zmq_bind (rbind, bindip);
    assert(0 == ret);

    //  Set up connection stream.
    rconn1 = zmq_socket (ctx_, ZMQ_STREAM);
    assert (rconn1);
    ret = zmq_setsockopt (rconn1, ZMQ_LINGER, &zero, sizeof (zero));
    assert (0 == ret);
    
    //  Do the connection.
    ret = zmq_setsockopt (rconn1, ZMQ_CONNECT_RID, "conn1", 6);
    assert (0 == ret);
    ret = zmq_connect (rconn1, bindip);

/*  Uncomment to test assert on duplicate rid.
    //  Test duplicate connect attempt.
    ret = zmq_setsockopt (rconn1, ZMQ_CONNECT_RID, "conn1", 6);
    assert (0 == ret);
    ret = zmq_connect (rconn1, bindip);
    assert (0 == ret);
*/   
    //  Send data to the bound stream.
    ret = zmq_send (rconn1, "conn1", 6, ZMQ_SNDMORE);
    assert (6 == ret);
    ret = zmq_send (rconn1, msg, 5, 0);
    assert (5 == ret);

    //  Accept data on the bound stream.
    ret = zmq_recv (rbind, buff, 256, 0);
    assert (ret && 0 == buff[0]);
    assert (0 == buff[0]);
    ret = zmq_recv (rbind, buff, 256, 0);
    assert (0 == ret);

    // Handle close of the socket.
    ret = zmq_recv (rbind, buff, 256, 0);
    assert (ret);
    assert (0 == buff[0]);
    ret = zmq_recv (rbind, buff+128, 128, 0);
    assert (5 == ret); 
    assert ('h' == buff[128]);

    ret = zmq_unbind (rbind, bindip);
    assert(0 == ret);
    ret = zmq_close (rbind);
    assert(0 == ret);
    ret = zmq_close (rconn1);
    assert(0 == ret);
}

void test_router_2_router(void* ctx,bool named){
    void *rbind, *rconn1;
    int ret;
    char buff[256];
    char msg[] = "hi 1";
    const char *bindip = "tcp://127.0.0.1:5556";
    int zero = 0;

    //  Create bind socket.
    rbind = zmq_socket (ctx, ZMQ_ROUTER);
    assert (rbind);
    ret = zmq_setsockopt (rbind, ZMQ_LINGER, &zero, sizeof (zero));
    assert (0 == ret);
    ret = zmq_bind (rbind, bindip);
    assert (0 == ret);

    //  Create connection socket.
    rconn1 = zmq_socket (ctx, ZMQ_ROUTER);
    assert (rconn1); 
    ret = zmq_setsockopt (rconn1, ZMQ_LINGER, &zero, sizeof (zero));
    assert (0 == ret);

    //  If we're in named mode, set some identities.
    if (named) {
        ret = zmq_setsockopt (rbind, ZMQ_IDENTITY, "X", 1);
        ret = zmq_setsockopt (rconn1, ZMQ_IDENTITY, "Y", 1);
    }

    //  Make call to connect using a connect_rid. 
    ret = zmq_setsockopt (rconn1, ZMQ_CONNECT_RID, "conn1", 6);
    assert (0 == ret);
    ret = zmq_connect (rconn1, bindip);
    assert (0 == ret);
/*  Uncomment to test assert on duplicate rid 
    //  Test duplicate connect attempt.
    ret = zmq_setsockopt (rconn1, ZMQ_CONNECT_RID, "conn1", 6);
    assert (0 == ret);
    ret = zmq_connect (rconn1, bindip);
    assert (0 == ret);
*/
    //  Send some data.
    ret = zmq_send (rconn1, "conn1", 6, ZMQ_SNDMORE);
    assert (6 == ret);
    ret = zmq_send (rconn1, msg, 5, 0);
    assert (5 == ret);

    //  Receive the name.
    ret = zmq_recv (rbind, buff, 256, 0);
    if (named) 
        assert (ret && 'Y' == buff[0]);
    else 
        assert (ret && 0 == buff[0]);

    //  Receive the data.
    ret = zmq_recv (rbind, buff+128, 128, 0);
    assert(5 == ret && 'h' == buff[128]);

    //  Send some data back.
    if (named) {
        ret = zmq_send (rbind, buff, 1, ZMQ_SNDMORE);
        assert (1 == ret);
    }
    else {
        ret = zmq_send (rbind, buff, 5, ZMQ_SNDMORE);
        assert (5 == ret);
    }
    ret = zmq_send_const (rbind, "ok", 3, 0);
    assert (3 == ret);
    
    //  If bound socket identity naming a problem, we'll likely see something funky here.
    ret = zmq_recv (rconn1, buff, 256, 0);
    assert ('c' == buff[0] && 6 == ret);
    ret = zmq_recv (rconn1, buff+128, 128, 0);
    assert (3 == ret && 'o' == buff[128]);

    ret = zmq_unbind (rbind, bindip);
    assert(0 == ret);
    ret = zmq_close (rbind);
    assert(0 == ret);
    ret = zmq_close (rconn1);
    assert(0 == ret);
}

int main (void)
{
    void *ctx; 
    setup_test_environment ();
    ctx = zmq_ctx_new ();
    assert (ctx);
    test_stream_2_stream (ctx);
    msleep(100);	// Give time for bound socket to be closed.
    test_router_2_router (ctx, false);
    msleep(100);	// Give time for bound socket to be closed.
    test_router_2_router (ctx, true);
    zmq_ctx_destroy (ctx);
    printf ("'test_connect_rid' passed");
    return 0;
}

