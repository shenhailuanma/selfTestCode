#include "server.h"
#include "smem.h"


void smemlistMoveNode(list *list_src, list *list_dst, listNode *node)
{
    // del from src list
    if (node->prev)
        node->prev->next = node->next;
    else
        list_src->head = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        list_src->tail = node->prev;

    list_src->len--;

    // add to dst list
    if (list_dst->len == 0) {
        list_dst->head = list_dst->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = NULL;
        node->next = list_dst->head;
        list_dst->head->prev = node;
        list_dst->head = node;
    }

    list_dst->len++;
}


int smemListMatch(void *ptr, void *key) {
    struct smem_t *smem_p = ptr;
    int * id = key;


    return (smem_p->id == *id);
}

void smemgetCommand(client *c)
{
    long long ll_size;
    int size;
    int mem_id = -1;
    listNode *ln;
    listIter li;
    struct smem_t * smem_p = NULL;

    // check the size

    if(getLongLongFromObject(c->argv[1],&ll_size) == C_OK){
        //serverLog(LL_WARNING,"[smemgetCommand] will get share memory size: %lld", ll_size);
        size = ll_size;
        
        // get buffer from list
        listRewind(server.smem_list_available, &li);
        while ((ln = listNext(&li)) != NULL) {
            smem_p = ln->value;

            // compare the size
            if((smem_p->state == SMEM_T_STATE_AVAILAVLE) && (size <= smem_p->size)){
                
                smem_p->state = SMEM_T_STATE_USED;
                smem_p->cnt ++;
                smem_p->last_time = server.unixtime;
                mem_id = smem_p->id;


                // move it to used list
                smemlistMoveNode(server.smem_list_available, server.smem_list_used, ln);


                break;
            } 
        }
        
        if(mem_id == -1){
            // check the share memory max limit
            if((server.share_memory_size + size) <= server.share_memory_limit){
                // get buffer by use smem
                mem_id = smem_get_buffer(size);

                if(mem_id != -1){
                    
                    smem_p = zmalloc(sizeof(struct smem_t));
                    if(smem_p == NULL){
                        mem_id = -1;
                        serverLog(LL_WARNING,"[smemgetCommand] zmalloc failed.");
                    }else{
                        smem_p->id = mem_id;
                        smem_p->size = size;
                        smem_p->state = SMEM_T_STATE_USED;
                        smem_p->cnt = 1;
                        smem_p->last_time = server.unixtime;
                        
                        // add itme to list
                        listAddNodeTail(server.smem_list_used, smem_p);

                        // update the share memory used status
                        server.share_memory_size += size;

                    }
                    
                }
            }else{
                serverLog(LL_WARNING,"[smemgetCommand] server.share_memory_size:%d + %d > server.share_memory_limit:%d", server.share_memory_size, size, server.share_memory_limit);
            }



        }
        
    }

    serverLog(LL_WARNING,"[smemfreeCommand] get share memory id: %d", mem_id);

    addReplyLongLong(c,mem_id);
    return C_OK;
}

void smemfreeCommand(client *c)
{
    long long ll_var;
    int mem_id;
    int free_cnt = 0;
    struct smem_t * smem_p = NULL;
    listNode *lnode;

    int j;

    for (j = 1; j < c->argc; j++){


        if(getLongLongFromObject(c->argv[j],&ll_var) == C_OK){
            //serverLog(LL_WARNING,"[smemfreeCommand] get share memory id: %lld", ll_var);
            mem_id = ll_var;

            // get the item from list
            lnode = listSearchKey(server.smem_list_used, &mem_id);
            if(lnode){
                // 
                smem_p = lnode->value;
                if(smem_p->cnt > 0){
                    smem_p->cnt--;
                }

                if(smem_p->cnt == 0){
                    smem_p->state = SMEM_T_STATE_AVAILAVLE;
                    // add the item to smem_list_available
                    smemlistMoveNode(server.smem_list_used, server.smem_list_available, lnode);
                }

                free_cnt ++;

            }
        }
        
    }



    addReplyLongLong(c,free_cnt);
    return C_OK;
}


void smemrmCommand(client *c)
{
    long long ll_var;
    int mem_id;
    int free_cnt = 0;
    struct smem_t * smem_p = NULL;
    listNode *lnode;

    int j;

    for (j = 1; j < c->argc; j++){
        // check the size
        if(getLongLongFromObject(c->argv[j],&ll_var) == C_OK){
            //serverLog(LL_WARNING,"get share memory id: %lld", ll_var);
            mem_id = ll_var;

            // get the item from list
            lnode = listSearchKey(server.smem_list_available, &mem_id);
            if(lnode){
                smem_p = lnode->value;

                // update the share memory used status
                server.share_memory_size -= smem_p->size;

                //serverLog(LL_WARNING,"[smemrmCommand] rm the id(%d) in list.", mem_id);
                listDelNode(server.smem_list_available, lnode);

            }else
                //serverLog(LL_WARNING,"[smemrmCommand] not found the id(%d) in list, try to free.", mem_id);

            if(!smem_free_buffer(mem_id)){
                free_cnt ++;
            }

        }
        
    }

    addReplyLongLong(c,free_cnt);
    return C_OK;
}


/* Unsubscribe a client from a channel. Returns 1 if the operation succeeded, or
 * 0 if the client was not subscribed to the specified channel. */
int smempubsubUnsubscribeChannel(client *c, robj *channel, int notify) 
{
    dictEntry *de;
    list *clients;
    listNode *ln;
    int retval = 0;

    /* Remove the channel from the client -> channels hash table */
    incrRefCount(channel); /* channel may be just a pointer to the same object
                            we have in the hash tables. Protect it... */
    if (dictDelete(c->smempubsub_channels,channel) == DICT_OK) {
        retval = 1;
        /* Remove the client from the channel -> clients list hash table */
        de = dictFind(server.smempubsub_channels,channel);
        serverAssertWithInfo(c,NULL,de != NULL);
        clients = dictGetVal(de);
        ln = listSearchKey(clients,c);
        serverAssertWithInfo(c,NULL,ln != NULL);
        listDelNode(clients,ln);
        if (listLength(clients) == 0) {
            /* Free the list and associated hash entry at all if this was
             * the latest client, so that it will be possible to abuse
             * Redis PUBSUB creating millions of channels. */
            dictDelete(server.smempubsub_channels,channel);
        }
    }
    /* Notify the client */
    if (notify) {
        addReply(c,shared.mbulkhdr[3]);
        addReply(c,shared.unsubscribebulk);
        addReplyBulk(c,channel);
        addReplyLongLong(c,dictSize(c->smempubsub_channels));

    }
    decrRefCount(channel); /* it is finally safe to release it */
    return retval;
}


/* Unsubscribe from all the channels. Return the number of channels the
 * client was subscribed to. */
int smempubsubUnsubscribeAllChannels(client *c, int notify) 
{
    dictIterator *di = dictGetSafeIterator(c->smempubsub_channels);
    dictEntry *de;
    int count = 0;

    while((de = dictNext(di)) != NULL) {
        robj *channel = dictGetKey(de);

        count += smempubsubUnsubscribeChannel(c,channel,notify);
    }
    /* We were subscribed to nothing? Still reply to the client. */
    if (notify && count == 0) {
        addReply(c,shared.mbulkhdr[3]);
        addReply(c,shared.unsubscribebulk);
        addReply(c,shared.nullbulk);
        addReplyLongLong(c,dictSize(c->smempubsub_channels));
    }
    dictReleaseIterator(di);
    return count;
}



/* Subscribe a client to a channel. Returns 1 if the operation succeeded, or
 * 0 if the client was already subscribed to that channel. */
int smempubsubSubscribeChannel(client *c, robj *channel) 
{
    dictEntry *de;
    list *clients = NULL;
    int retval = 0;


    /* Add the channel to the client -> channels hash table */
    if (dictAdd(c->smempubsub_channels,channel,NULL) == DICT_OK) {
        retval = 1;
        incrRefCount(channel);

        /* Add the client to the channel -> list of clients hash table */
        de = dictFind(server.smempubsub_channels,channel);
        if (de == NULL) {
            clients = listCreate();

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


void smemsubscribeCommand(client *c) 
{
    int j;

    for (j = 1; j < c->argc; j++)
        smempubsubSubscribeChannel(c,c->argv[j]);
    c->flags |= CLIENT_PUBSUB;
}


/* Publish a message */
int smempubsubPublishMessage(robj *channel, robj *message) 
{
    int receivers = 0;
    dictEntry *de;
    listNode *ln;
    listIter li;
    int mem_id = -1;
    struct smem_t * smem_p = NULL;
    listNode *lnode = NULL;
    long long ll_var;

    /* Get the share memory id */
    if(getLongLongFromObject(message,&ll_var) == C_OK){
        //serverLog(LL_WARNING,"[smempubsubPublishMessage] get share memory id: %lld", ll_var);
        mem_id = ll_var;


        // get the item from list
        lnode = listSearchKey(server.smem_list_used, &mem_id);
        if(lnode){
            smem_p = lnode->value;
        }else{
            //serverLog(LL_WARNING,"[smempubsubPublishMessage] not found the id(%d) in list.", mem_id);
            mem_id = -1;
        }
    }


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

            // if message is share memory id, should increase use count
            if(lnode){
                smem_p->cnt++;
                //serverLog(LL_WARNING,"[smempubsubPublishMessage] receivers:%d, smem_p->cnt=%d.", receivers, smem_p->cnt);
            }
        }
    }

    return receivers;
}


void smempublishCommand(client *c) 
{
    int receivers = smempubsubPublishMessage(c->argv[1],c->argv[2]);
    if (server.cluster_enabled)
        clusterPropagatePublish(c->argv[1],c->argv[2]);
    else
        forceCommandPropagation(c,PROPAGATE_REPL);
    addReplyLongLong(c,receivers);
}


void smemFreeShareMemory(int timeout)
{

    int mem_id = -1;
    listNode *ln;
    listIter li;
    struct smem_t * smem_p = NULL;
    int available_free_cnt = 0, used_free_cnt = 0;

    // get buffer from list
    listRewind(server.smem_list_available, &li);
    while ((ln = listNext(&li)) != NULL) {
        smem_p = ln->value;

        if( (server.unixtime - smem_p->last_time) > timeout){
            mem_id = smem_p->id;

            server.share_memory_size -= smem_p->size;

            // delete the node
            listDelNode(server.smem_list_available, ln);

            // free the share memory
            smem_free_buffer(mem_id);

            available_free_cnt++;
        }
    }

    timeout += 5;
    listRewind(server.smem_list_used, &li);
    while ((ln = listNext(&li)) != NULL) {
        smem_p = ln->value;

        if( (server.unixtime - smem_p->last_time) > timeout){
            mem_id = smem_p->id;
            server.share_memory_size -= smem_p->size;

            // delete the node
            listDelNode(server.smem_list_used, ln);

            // free the share memory
            smem_free_buffer(mem_id);
            used_free_cnt++;
        }
    }
    serverLog(LL_WARNING,"[smemFreeShareMemory] smem_list_available:%d, available_free_cnt=%d, smem_list_used:%d, used_free_cnt=%d.", 
        server.smem_list_available->len, available_free_cnt, server.smem_list_used->len, used_free_cnt);
}