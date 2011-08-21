#include "CouchDB.h"
#include <curl/curl.h>
#include <string>
#include <memory>
#include "EZ.h"

using namespace std;

namespace CouchDB {

static const char *url_sep_after(const string &s)
{
    if (s.length() && s[s.length() - 1] != '/')
        return "/";
    else
        return "";
}

Server::Server(const string &url) : url(url + url_sep_after(url)) {}
Database::Database(Server &server, string &db) 
    : server(server), url(server.url + db + url_sep_after(db)) {}

string Server::next_uuid()
{
    EZ::MutexLock lock(uuid_cache_mutex);
    string uuid;

    if (uuid_cache.size())
    {
        uuid = uuid_cache.back();
        uuid_cache.pop_back();
        return uuid;
    }
    else
    {
        string uuid_url(url);
        uuid_url.append("_uuids?count=100");

        string *response = curl.get(uuid_url);
        auto_ptr<string> response_destroyer(response);

        Json::Reader reader;
        Json::Value root;
        
        if (!reader.parse(*response, root, false))
            throw "JSON Parsing error";

        response_destroyer.reset();

        const Json::Value uuids = root["uuids"];
        if (!uuids.isArray() || !uuids.size())
            throw "Invalid UUIDs response";

        uuid = uuids[Json::UInt(0)].asString();

        for (Json::UInt index = 1; index < uuids.size(); index++)
            uuid_cache.push_back(uuids[index].asString());
    }

    return uuid;
}

Json::Value *Database::operator[](const string &doc_id)
{
    return get_doc(doc_id);
}

void Database::save_doc(Json::Value &doc)
{
    Json::Value &id = doc["_id"];

    if (id.isNull())
    {
        id = server.next_uuid();
    }
    else if (!id.isString())
    {
        throw "_id must be a string if set";
    }

    /* TODO: save it. */
}

Json::Value *Database::get_doc(const string &doc_id)
{
    string doc_url(url);
    string *doc_id_escaped = EZ::cURL::escape(doc_id);

    doc_url.append(*doc_id_escaped);
    delete doc_id_escaped;

    Json::Reader reader;
    Json::Value *doc = new Json::Value;
    auto_ptr<Json::Value> value_destroyer(doc);

    string *response = server.curl.get(doc_url);
    auto_ptr<string> response_destroyer(response);

    if (!reader.parse(*response, *doc, false))
        throw "JSON Parsing error";

    response_destroyer.reset();
    value_destroyer.release();

    return doc;
}

} /* namespace CouchDB */
