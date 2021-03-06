#pragma once

#include <zk/config.hpp>

#include <initializer_list>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include "forwards.hpp"

namespace zk
{

/** Describes the ability of a user to perform a certain action. Permissions can be mixed together like integers with
 *  \c | and \c &.
**/
enum class permission : unsigned int
{
    none    = 0b00000, //!< No permissions are set (server could have been configured without ACL support).
    read    = 0b00001, //!< You can access the data of a node and can list its children.
    write   = 0b00010, //!< You can set the data of a node.
    create  = 0b00100, //!< You can create a child node.
    erase   = 0b01000, //!< You can erase a child node (but not necessarily this one).
    admin   = 0b10000, //!< You can alter permissions on this node.
    all     = 0b11111, //!< You can do anything.
};

constexpr permission operator|(permission a, permission b)
{
    return permission(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

constexpr permission operator&(permission a, permission b)
{
    return permission(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

constexpr permission operator~(permission a)
{
    return permission(~static_cast<unsigned int>(a));
}

/** Check that \a self allows it \a perform all operations. For example,
 *  `allows(permission::read | permission::write, permission::read)` will be \c true, as `read|write` is allowed to
 *  \c read.
**/
constexpr bool allows(permission self, permission perform)
{
    return (self & perform) == perform;
}

std::ostream& operator<<(std::ostream&, const permission&);

std::string to_string(const permission&);

/** An access control list. In general, the ACL system is similar to UNIX file access permissions, where znodes act as
 *  files. Unlike UNIX, each znode can have any number of ACLs to correspond with the potentially limitless (and
 *  pluggable) authentication schemes. A more surprising difference is that ACLs are not recursive: If \c /path is only
 *  readable by a single user, but \c /path/sub is world-readable, then anyone will be able to read \c /path/sub.
 *
 *  \see https://zookeeper.apache.org/doc/trunk/zookeeperProgrammers.html#sc_ZooKeeperAccessControl
**/
class acl final
{
public:
    /** Create an ACL under the given \a scheme and \a id with the given \a permissions. **/
    acl(std::string scheme, std::string id, permission permissions);

    ~acl() noexcept;

    /** The authentication scheme this list is used for. The most common scheme is `"auth"`, which allows any
     *  authenticated user to perform actions (see \c acls::creator_all).
    **/
    const std::string& scheme() const
    {
        return _scheme;
    }

    /** The ID of the user under the \c scheme. For example, with the \c "ip" \c scheme, this is an IP address or CIDR
     *  netmask.
    **/
    const std::string& id() const
    {
        return _id;
    }

    /** The permissions associated with this ACL. **/
    const permission& permissions() const
    {
        return _permissions;
    }

private:
    std::string _scheme;
    std::string _id;
    permission  _permissions;
};

std::size_t hash(const acl&);

[[gnu::pure]] bool operator==(const acl& lhs, const acl& rhs);
[[gnu::pure]] bool operator!=(const acl& lhs, const acl& rhs);
[[gnu::pure]] bool operator< (const acl& lhs, const acl& rhs);
[[gnu::pure]] bool operator<=(const acl& lhs, const acl& rhs);
[[gnu::pure]] bool operator> (const acl& lhs, const acl& rhs);
[[gnu::pure]] bool operator>=(const acl& lhs, const acl& rhs);

std::ostream& operator<<(std::ostream&, const acl&);

std::string to_string(const acl&);

/** A list of ACLs. In general, this is a wrapper around a \c std::vector of \c acl instances. **/
class acl_list final
{
public:
    using iterator       = std::vector<acl>::iterator;
    using const_iterator = std::vector<acl>::const_iterator;

public:
    acl_list(std::vector<acl> acls) noexcept;

    acl_list(std::initializer_list<acl> acls) :
            acl_list(std::vector<acl>(acls))
    { }

    ~acl_list() noexcept;

    iterator begin()              { return _impl.begin(); }
    const_iterator begin() const  { return _impl.begin(); }
    const_iterator cbegin() const { return _impl.begin(); }

    iterator end()              { return _impl.end(); }
    const_iterator end() const  { return _impl.end(); }
    const_iterator cend() const { return _impl.end(); }

private:
    std::vector<acl> _impl;
};

std::ostream& operator<<(std::ostream&, const acl_list&);

std::string to_string(const acl_list& self);

/** Convenience operations around commonly-used ACLs. **/
class acls
{
public:
    /** This ACL gives the creators authentication id's all permissions. **/
    static const acl_list& creator_all();

    /** This is a completely open ACL. **/
    static const acl_list& open_unsafe();

    /** This ACL gives the world the ability to read. **/
    static const acl_list& read_unsafe();
};

}

namespace std
{

template <>
class hash<zk::acl>
{
public:
    using argument_type = zk::acl;
    using result_type   = std::size_t;

    result_type operator()(const argument_type& x) const
    {
        return zk::hash(x);
    }
};

}
