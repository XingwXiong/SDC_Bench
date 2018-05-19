#include <cstring>
#include <iostream>
#include <sstream>

#include <assert.h>
#include <unistd.h>

#include "server.h"
#include "tbench_server.h"
#include "helpers.h"

using namespace std;

unsigned long Server::numReqsToProcess = 0;
volatile atomic_ulong Server::numReqsProcessed(0);
pthread_barrier_t Server::barrier;

Server::Server(int id, string dbPath) 
    : db(dbPath)
    , enquire(db)
    , stemmer("english")
    , id(id)
{
    const char* stopWords[] = { "a", "about", "an", "and", "are", "as", "at", "be",
        "by", "en", "for", "from", "how", "i", "in", "is", "it", "of", "on",
        "or", "that", "the", "this", "to", "was", "what", "when", "where",
        "which", "who", "why", "will", "with" };

    stopper = Xapian::SimpleStopper(stopWords, \
            stopWords + sizeof(stopWords) / sizeof(stopWords[0]));

    parser.set_database(db);
    parser.set_default_op(Xapian::Query::OP_OR);
    parser.set_stemmer(stemmer);
    parser.set_stemming_strategy(Xapian::QueryParser::STEM_SOME);
    parser.set_stopper(&stopper);
}

Server::~Server() {
}

void Server::_run() {
    pthread_barrier_wait(&barrier);
	
    tBenchServerThreadStart();
    while (numReqsProcessed < numReqsToProcess) {
        processRequest();
        ++numReqsProcessed;
    }
}

void Server::processRequest() {
    const unsigned MAX_TERM_LEN = 256;
    char term[MAX_TERM_LEN];
    void* termPtr;
    // 阻塞地获取来自client的请求
    size_t len = tBenchRecvReq(&termPtr);
    memcpy(reinterpret_cast<void*>(term), termPtr, len);
    term[len] = '\0';
    
    // enquire -- 查询会话
    uint64_t delta = getCurNs();
    unsigned int flags = Xapian::QueryParser::FLAG_DEFAULT;
    // 解析待查找字符串
    Xapian::Query query = parser.parse_query(term, flags);
    // 将解析后的字符串加入会话
    enquire.set_query(query);
    // 得到查询结果
    mset = enquire.get_mset(0, MSET_SIZE);
    delta = getCurNs() - delta;

    const unsigned MAX_RES_LEN = 1 << 20;
    char* res=new char[MAX_RES_LEN];
    unsigned resLen = 0;
    unsigned doccount = 0;
    const unsigned MAX_DOC_COUNT = 25; // up to 25 results per page
    // 将结果输出到res 数组。
    for (auto it = mset.begin(); it != mset.end(); ++it) {
        std::string desc = it.get_document().get_description();
        resLen += desc.size();
        assert(resLen <= MAX_RES_LEN);
        memcpy(reinterpret_cast<void*>(&res[resLen]), desc.c_str(), desc.size());
        if (++doccount == MAX_DOC_COUNT) break;
    }
    assert(resLen < MAX_RES_LEN);
    // 发送数据到client
    tBenchSendResp(reinterpret_cast<void*>(res), resLen, delta);
    delete []res;
}

void* Server::run(void* v) {
    Server* server = static_cast<Server*> (v);
    server->_run();
    return NULL;
}

void Server::init(unsigned long _numReqsToProcess, unsigned numServers) {
    numReqsToProcess = _numReqsToProcess;
    pthread_barrier_init(&barrier, NULL, numServers);
}

void Server::fini() {
    pthread_barrier_destroy(&barrier);
}
