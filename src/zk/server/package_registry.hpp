#pragma once

#include <zk/config.hpp>
#include <zk/optional.hpp>

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace zk::server
{

/** The package registry tracks configuration of classpaths and JARs needed to run various ZooKeeper versions.
 *
 *  \note{Thread Safety}
 *  Registering and unregistering configurations is thread-safe. However, it is \e not safe when a \c package_registry
 *  is being destroyed.
**/
class package_registry final
{
public:
    using size_type = std::size_t;

    struct registration_info;
    using registration = std::shared_ptr<registration_info>;

public:
    package_registry();

    ~package_registry() noexcept;

    /** Register a server that can be created via the specified Java \a classpath.
     *
     *  \param version A version string used to look up the server when creating them. While this can be a lie, it
     *   should not be.
     *  \param classpath The Java classpath used to run the server. This will be the \c cp argument to Java.
     *  \returns a registration that can be used to \c unregister_server.
     *  \throws std::invalid_argument if \a version is already registered.
    **/
    registration register_classpath_server(std::string version, std::string classpath);

    /** Attempt to unregister the server associated with the provided registration. Unregistering will prevent future
     *  servers from being created with the particular setup, but will not teardown servers which might be running with
     *  it.
     *
     *  \returns \c true if this call removed anything; \c false if otherwise.
    **/
    bool unregister_server(registration reg);
    bool unregister_server(const registration_info& reg);

    /** How many registrations have been registered? **/
    size_type size() const;

    /** Is this registry empty? **/
    bool empty() const
    {
        return size() == size_type(0);
    }

    /** Get the classpath for running the newest registered server version.
     *
     *  \warning
     *  This function is nonsense and will be deprecated. The concept is fine, but returning an \c std::string as a
     *  classpath is terrible. It should be replaced by a dedicated \c run_settings class.
    **/
    optional<std::string> find_newest_classpath() const;

private:
    mutable std::mutex                 _protect;
    std::shared_ptr<void>              _lifetime;
    std::map<std::string, std::string> _registrations;
};

}
