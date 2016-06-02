#include "server.h"
#include "smem.h"


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
        serverLog(LL_WARNING,"will get share memory size: %lld", ll_size);
        size = ll_size;
        
        // get buffer from list
        listRewind(server.smem_list,&li);
        while ((ln = listNext(&li)) != NULL) {
            smem_p = ln->value;

            // compare the size
            if((smem_p->state == SMEM_T_STATE_AVAILAVLE) && (size <= smem_p->size)){
                
                smem_p->state = SMEM_T_STATE_USED;
                smem_p->cnt ++;

                mem_id = smem_p->id;

                break;
            } 
        }
        
        if(mem_id == -1){
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

                    // add itme to list
                    listAddNodeTail(server.smem_list, smem_p);
                }
                
            }
        }
        
    }

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
        // check the size
        if(getLongLongFromObject(c->argv[j],&ll_var) == C_OK){
            serverLog(LL_WARNING,"get share memory id: %lld", ll_var);
            mem_id = ll_var;

            serverLog(LL_WARNING,"[smemfreeCommand] smem_list len=%d.", listLength(server.smem_list));

            // get the item from list
            lnode = listSearchKey(server.smem_list, &mem_id);
            if(lnode){
                // 
                smem_p = lnode->value;
                if(smem_p->cnt > 0){
                    smem_p->cnt--;
                }

                if(smem_p->cnt == 0){
                    smem_p->state = SMEM_T_STATE_AVAILAVLE;
                }

                free_cnt ++;
            }else{
                serverLog(LL_WARNING,"[smemfreeCommand] not found the id(%d) in list.", mem_id);

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
            serverLog(LL_WARNING,"get share memory id: %lld", ll_var);
            mem_id = ll_var;

            //serverLog(LL_WARNING,"[smemrmCommand] smem_list len=%d.", listLength(server.smem_list));

            // get the item from list
            lnode = listSearchKey(server.smem_list, &mem_id);
            if(lnode){
                serverLog(LL_WARNING,"[smemrmCommand] rm the id(%d) in list.", mem_id);
                listDelNode(server.smem_list, lnode);

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



/* Subscribe a client to a channel. Returns 1 if the operation succeeded, or
 * 0 if the client was already subscribed to that channel. */
int smempubsubSubscribeChannel(client *c, robj *channel) {
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