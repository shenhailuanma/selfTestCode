#include "server.h"
#include "smem.h"

void smemgetCommand(client *c)
{
    robj *o;
    // check the size

    int mem_id = smem_get_buffer(1234);

    //o = createStringObject("1", strlen("1"));
    //o = createStringObjectFromLongLong(mem_id);
    //addReply(c,o);
    addReplyLongLong(c,mem_id);
    return C_OK;
}

/* Subscribe a client to a channel. Returns 1 if the operation succeeded, or
 * 0 if the client was already subscribed to that channel. */
int smempubsubSubscribeChannel(client *c, robj *channel) {
    dictEntry *de;
    list *clients = NULL;
    int retval = 0;

    MY_LOG_DEBUG("c->smempubsub_channels=%lld\n", c->smempubsub_channels);
    /* Add the channel to the client -> channels hash table */
    if (dictAdd(c->smempubsub_channels,channel,NULL) == DICT_OK) {
        retval = 1;
        incrRefCount(channel);
        MY_LOG_DEBUG("server.smempubsub_channels=%lld\n", server.smempubsub_channels);
        /* Add the client to the channel -> list of clients hash table */
        de = dictFind(server.smempubsub_channels,channel);
        if (de == NULL) {
            clients = listCreate();
            MY_LOG_DEBUG("\n");
            dictAdd(server.smempubsub_channels,channel,clients);
            incrRefCount(channel);
        } else {
            clients = dictGetVal(de);
        }
        listAddNodeTail(clients,c);
    }
    /* Notify the client */
    addReply(c,shared.mbulkhdr[3]);
    addReply(c,shared.subscribebulk);
    addReplyBulk(c,channel);
    addReplyLongLong(c,dictSize(c->pubsub_channels));
    return retval;
}


void smemsubscribeCommand(client *c) {
    int j;

    for (j = 1; j < c->argc; j++)
        smempubsubSubscribeChannel(c,c->argv[j]);
    c->flags |= CLIENT_PUBSUB;
}


/* Publish a message */
int smempubsubPublishMessage(robj *channel, robj *message) {
    int receivers = 0;
    dictEntry *de;
    listNode *ln;
    listIter li;

    /* Send to clients listening for that channel */
    de = dictFind(server.smempubsub_channels,channel);
    if (de) {
        list *list = dictGetVal(de);
        listNode *ln;
        listIter li;

        listRewind(list,&li);
        while ((ln = listNext(&li)) != NULL) {
            client *c = ln->value;

            addReply(c,shared.mbulkhdr[3]);
            addReply(c,shared.messagebulk);
            addReplyBulk(c,channel);
            addReplyBulk(c,message);
            receivers++;
        }
    }

    return receivers;
}


void smempublishCommand(client *c) {
    int receivers = smempubsubPublishMessage(c->argv[1],c->argv[2]);
    if (server.cluster_enabled)
        clusterPropagatePublish(c->argv[1],c->argv[2]);
    else
        forceCommandPropagation(c,PROPAGATE_REPL);
    addReplyLongLong(c,receivers);
}