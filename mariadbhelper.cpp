#include "mariadbhelper.h"

MariadbHelper::MariadbHelper(const std::string &user,
                             const std::string &password,
                             const std::string &database,
                             const std::string &host,
                             unsigned int port,
                             bool autoConnect)
    : m_user(user), m_password(password), m_database(database), m_host(host), m_port(port), m_autoConnect(autoConnect)
{
    m_connected = false;
    if (m_autoConnect)
    {
        int ret = connectMariadb();
        std::cout << "auto con db: " << ret << std::endl;
    }
    std::cout << "using mariadbhelper constructor" << std::endl;
}
MariadbHelper::~MariadbHelper()
{
    disconnectMariadb();
    std::cout << "using mariadbhelper destructed" << std::endl;
}

int MariadbHelper::connectMariadb()
{
    if (m_conn)
    {
        std::cout<< "database connected" <<std::endl;
        return 0;
    }
    try
    {
        sql::SQLString url = "jdbc:mariadb://" + m_host + ":" + std::to_string(m_port) + "/" + m_database;
        sql::Properties properties({{"user", m_user}, {"password", m_password}});
        sql::Driver *driver = sql::mariadb::get_driver_instance();
        m_conn = std::unique_ptr<sql::Connection>(driver->connect(url, properties));
        if (!m_conn)
        {
            return -1;
        }
        m_connected = true;
    }
    catch (sql::SQLException &e)
    {
        m_lastError = e.what();
        m_connected = false;
        return -1;
    }
    return 0;
}
void MariadbHelper::disconnectMariadb()
{
    if (m_conn)
    {
        m_conn->close();
        m_conn.reset();
        m_connected = false;
    }
    else
    {
        std::cout << "disconnect mariadb did nothing, connect is not availiable" << std::endl;
    }
}

bool MariadbHelper::isConnected()
{
    return m_connected;
}

std::vector<std::map<std::string, sql::SQLString>> MariadbHelper::query(
    const std::string sql, const std::vector<sql::SQLString> &params)
{
    std::vector<std::map<std::string, sql::SQLString>> result;
    if (!m_connected)
    {
        std::cout << "not connected detected while query" << std::endl;
        return result;
    }
    std::unique_ptr<sql::PreparedStatement> stmnt(m_conn->prepareStatement(sql));
    for (size_t i = 0; i < params.size(); ++i)
    {
        stmnt->setString(i + 1, params[i]);
    }
    std::unique_ptr<sql::ResultSet> res(stmnt->executeQuery());
    sql::ResultSetMetaData *meta = res->getMetaData();
    unsigned int columns = meta->getColumnCount();
    while (res->next())
    {
        std::map<std::string, sql::SQLString> row;
        for (unsigned int i = 1; i <= columns; ++i)
        {
            std::string columnName = meta->getColumnName(i).c_str();
            row[columnName] = res->getString(i);
        }
        result.push_back(row);
    }
    return result;
}

bool MariadbHelper::execute(const std::string &sql, const std::vector<sql::SQLString> &params)
{
    if (!m_conn)
    {
        std::cout<< "error on execute sql, " <<std::endl;
        return false;
    }
    std::unique_ptr<sql::PreparedStatement> stmnt(m_conn->prepareStatement(sql));
    for (size_t i = 0; i < params.size(); ++i)
    {
        stmnt->setString(i + 1, params[i]);
    }
    stmnt->execute();
    return true;
}

uint64_t MariadbHelper::getLastInsertId() const
{
    if (!m_connected)
    {
        return 0;
    }
    try
    {
        std::unique_ptr<sql::Statement> stmt(m_conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID()"));
        if (res->next())
        {
            return res->getUInt64(1);
        }
    }
    catch (sql::SQLException &)
    {
        // 忽略错误
    }
    return 0;
}
std::string MariadbHelper::getLastError() const
{
    return m_lastError;
}

void MariadbHelper::beginTransaction()
{
    if (m_conn)
    {
        m_conn->setAutoCommit(false);
    }
    else
    {
        std::cout << "begin transaction error, nothing happened" << std::endl;
    }
}

void MariadbHelper::commit()
{
    if (m_conn)
    {
        m_conn->commit();
        m_conn->setAutoCommit(true);
    }
    else
    {
        std::cout << "commit transaction error, nothing happened" << std::endl;
    }
}

void MariadbHelper::rollback()
{
    if (m_conn)
    {
        m_conn->rollback();
        m_conn->setAutoCommit(true);
    }
    else
    {
        std::cout << "rollback transaction error, nothing happened" << std::endl;
    }
}
