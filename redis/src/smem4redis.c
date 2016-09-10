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

void smemlistAddNodeTail(list *list_dst, listNode *node)
{
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

void smemlistMoveNodeToDict(list *list_src, dict *dict_dst, listNode *node)
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

    node->prev = NULL;
    node->next = NULL;

    struct smem_t * smem_p = node->value;

    // add to dst list
    dictEntry *de;
    de = dictFind(dict_dst,createObject(OBJ_STRING,sdsfromlonglong(smem_p->id)));
    if(de == NULL){
        dictAdd(dict_dst, createObject(OBJ_STRING,sdsfromlonglong(smem_p->id)), node);
    }

}

void smemDictMoveNodeToList(dict *dict_src, list * list_dst, listNode *node)
{
    // delete from dict

    // add to list
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

    robj *smem_size_obj = c->argv[1];


    // check the size

    if(getLongLongFromObject(smem_size_obj, &ll_size) == C_OK){
        //serverLog(LL_WARNING,"[smemgetCommand] will get share memory size: %lld", ll_size);
        size = ll_size;
        
        // get buffer from list
        /*
        listRewind(server.smem_list_available, &li);
        while ((ln = listNext(&li)) != NULL) {
            smem_p = ln->value;

            //serverLog(LL_WARNING,"[smemgetCommand] 0, state=%d, cnt=%d, size=%d", smem_p->state, smem_p->cnt, smem_p->size);
            // compare the size
            if((smem_p->state == SMEM_T_STATE_AVAILAVLE) && (size <= smem_p->size)){
                
                smem_p->state = SMEM_T_STATE_USED;
                smem_p->cnt = 1;
                smem_p->free_cnt = 0;
                smem_p->last_time = server.unixtime;
                mem_id = smem_p->id;

                // move it to memory dict
                smemlistMoveNodeToDict(server.smem_list_available, server.smempubsub_memorys, ln);

                break;
            } 
        }
        */

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
                        smem_p->free_cnt = 0;
                        smem_p->last_time = server.unixtime;
                        

                        // add to dict
                        listNode *node = zmalloc(sizeof(*node));

                        if(node){
                            node->value = smem_p;

                            //robj * smem_obj = createStringObjectFromLongLong(smem_p->id);
                            robj * smem_obj = createObject(OBJ_STRING,sdsfromlonglong(smem_p->id));
                            incrRefCount(smem_obj);


                            dictAdd(server.smempubsub_memorys, smem_obj, node);

                            // update the share memory used status
                            server.share_memory_size += size;
                        }else{
                            mem_id = -1;
                            serverLog(LL_WARNING,"[smemgetCommand] listNode zmalloc failed.");
                            zfree(smem_p);
                        }


                    }
                    
                }
            }else{
                serverLog(LL_WARNING,"[smemgetCommand] server.share_memory_size:%d + %d > server.share_memory_limit:%d", server.share_memory_size, size, server.share_memory_limit);
            }



        }
        
    }

    //serverLog(LL_WARNING,"[smemgetCommand] get share memory id: %d", mem_id);

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
    int ret;

    robj *smem_obj = NULL;
    dictEntry *de;


    for (j = 1; j < c->argc; j++){
        smem_obj = c->argv[j];

        // get the info from dict by key
        de = dictFind(server.smempubsub_memorys, smem_obj);

        if(de){
            lnode = dictGetVal(de);

            if(lnode){
                smem_p = lnode->value;
                smem_p->cnt--;

                if(smem_p->cnt <= 0 && smem_p->state == SMEM_T_STATE_USED){
                    smem_p->state = SMEM_T_STATE_AVAILAVLE;
                    smem_p->cnt = 0;
                    smem_p->last_time = server.unixtime;

                    // add the item to smem_list_available
                    //smemlistAddNodeTail(server.smem_list_available, lnode);
                    //smemlistMoveNode(server.smem_list_used, server.smem_list_available, lnode);

                    // for test , to free
                    server.share_memory_size -= smem_p->size;
                    smem_free_buffer(smem_p->id);
                    zfree(smem_p);
                    zfree(lnode);
                    dictDelete(server.smempubsub_memorys, smem_obj);

                }

                free_cnt ++;
            }
            
        }else{
            if(getLongLongFromObject(smem_obj,&ll_var) == C_OK){
                mem_id = ll_var;
                smem_free_buffer(mem_id);
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
                serverLog(LL_WARNING,"[smemrmCommand] not found the id(%d) in list, try to free.", mem_id);

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
    de = dictFind(server.smempubsub_memorys,message);
    if(de){
        lnode = dictGetVal(de);

        if(lnode){
            smem_p = lnode->value;
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
    int ret;
    long long ll_var;

    // get buffer from list
    listRewind(server.smem_list_available, &li);
    while ((ln = listNext(&li)) != NULL) {
        smem_p = ln->value;

        if( (server.unixtime - smem_p->last_time) > timeout){
            mem_id = smem_p->id;

            server.share_memory_size -= smem_p->size;

            // free the share memory
            ret = smem_free_buffer(mem_id);
            if(ret == 0){
                // delete the node
                listDelNode(server.smem_list_available, ln);
                available_free_cnt++;

                dictDelete(server.smempubsub_memorys, createObject(OBJ_STRING,sdsfromlonglong(mem_id)));

            }else{
                smem_p->free_cnt++;
                if(smem_p->free_cnt > 5){
                    listDelNode(server.smem_list_available, ln);
                    dictDelete(server.smempubsub_memorys, createObject(OBJ_STRING,sdsfromlonglong(mem_id)));
                }
            }

        }
    }
   
    /*
    dictIterator *di = dictGetSafeIterator(server.smempubsub_memorys);
    //dictIterator *di = dictGetIterator(server.smempubsub_memorys);
    dictEntry *de;
    while((de = dictNext(di)) != NULL) {
        ln = dictGetVal(de);
        robj * key = dictGetKey(de);

        //serverLog(LL_WARNING,"[smemFreeShareMemory] 1.");

        getLongLongFromObject(key,&ll_var);

        //de = dictFind(server.smempubsub_memorys, key);
        //if(de == NULL){
        //    serverLog(LL_WARNING,"[smemFreeShareMemory] not get key.");
        //    continue;
        //}
        //ln = dictGetVal(de);

        smem_p = ln->value;

        //serverLog(LL_WARNING,"[smemFreeShareMemory] 1, id=%d, size=%d, state=%d, cnt=%d, free_cnt=%d, last_time=%d, ll_var=%lld", 
        //    smem_p->id, smem_p->size, smem_p->state, smem_p->cnt, smem_p->free_cnt, smem_p->last_time, ll_var );
        if( (server.unixtime - smem_p->last_time) > timeout){
            mem_id = smem_p->id;
            server.share_memory_size -= smem_p->size;
            //serverLog(LL_WARNING,"[smemFreeShareMemory] 2, mem_id=%d.", mem_id);
            // free the share memory
            ret = smem_free_buffer(mem_id);
            if(ret == 0){
                //serverLog(LL_WARNING,"[smemFreeShareMemory] 3.");
                zfree(smem_p);
                zfree(ln);
                //serverLog(LL_WARNING,"[smemFreeShareMemory] 4.");
                // delete the node
                dictDelete(server.smempubsub_memorys, dictGetKey(de));
                //serverLog(LL_WARNING,"[smemFreeShareMemory] 5.");
                used_free_cnt++;
            }else{
                //serverLog(LL_WARNING,"[smemFreeShareMemory] 6.");
                smem_p->free_cnt++;
                if(smem_p->free_cnt > 5){
                    zfree(smem_p);
                    zfree(ln);
                    // delete the node
                    dictDelete(server.smempubsub_memorys, dictGetKey(de));
                }
            }
            
        }

    }
    dictReleaseIterator(di);
    */

    serverLog(LL_NOTICE,"[smemFreeShareMemory] smem_list_available:%d, available_free_cnt=%d, dictSize:%d, used_free_cnt=%d.", 
        server.smem_list_available->len, available_free_cnt, dictSize(server.smempubsub_memorys), used_free_cnt);
}