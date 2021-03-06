#pragma once

#include <zk/config.hpp>
#include <zk/forwards.hpp>
#include <zk/tests/test.hpp>

#include <memory>

namespace zk::server
{

class server;

class server_fixture :
        public test::test_fixture
{
public:
    virtual void SetUp() override;

    virtual void TearDown() override;

protected:
    const std::string& get_connection_string() const;

private:
    std::shared_ptr<server> _server;
    std::string             _conn_string;
};

}
