

#ifndef _IPC_SYNC_ROBUST_EXCLUSIVE_LOCK_H_
#define _IPC_SYNC_ROBUST_EXCLUSIVE_LOCK_H_

#ifdef  _MSC_VER
#include <io.h>
#else
#include <sys/file.h>
#endif // ifdef  _MSC_VER

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace ipc
{
namespace detail
{

const std::string default_shared_dir = "/dev/shm";

static std::string get_shared_file_full_path(const std::string &file_name)
{
    return default_shared_dir + "/" + file_name;
}

/**
 * This class implement an interprocess named resource that holds a mutual exclusion lock until
 * destroyed, or until the creator process dies
 */
class RobustExclusiveLock
{
public:

    /**
     * Open or create and acquire the interprocess lock.
     * @param in name Is the object's interprocess global name, visible for all processes in the same machine.
     * @param out was_lock_created If the lock succeeded, this parameter return whether the lock has been created
     * or it already exist.
     * @throw std::exception if lock coulnd't be acquired
     */
    RobustExclusiveLock(
            const std::string& name,
            bool* was_lock_created)
    {
        auto file_path = get_shared_file_full_path(name);

        if ((fd_ = open_and_lock_file(file_path, was_lock_created)) == -1)
        {
            throw std::runtime_error("open_and_lock_file failed");
        }

        name_ = name;
    }

    /**
     * Open or create and acquire the interprocess lock.
     * @param in name Is the object interprocess global name, visible for all processes in the same machine.
     * @throw std::exception if lock coulnd't be acquired
     */
    RobustExclusiveLock(
            const std::string& name)
    {
        bool was_lock_created;

        auto file_path = get_shared_file_full_path(name);

        if ((fd_ = open_and_lock_file(file_path, &was_lock_created)) == -1)
        {
            throw std::runtime_error("open_and_lock_file failed");
        }

        name_ = name;
    }

    /**
     * Checks whether the file is locked.
     * @param in name Is the object interprocess global name, visible for all processes in the same machine.
     */
    static bool is_locked(
            const std::string& name)
    {
        bool was_lock_created;

        auto file_path = get_shared_file_full_path(name);

        int fd;
        if ((fd = open_and_lock_file(file_path, &was_lock_created)) == -1)
        {
            return true;
        }

        unlock_and_close(fd, name);

        return false;
    }

    /**
     *  Unlock the interprocess lock.
     */
    ~RobustExclusiveLock()
    {
        unlock_and_close(fd_, name_);
    }

    /**
     * Remove the object
     * @return true when success, false otherwise.
     */
    static bool remove(
            const std::string& name)
    {
        return 0 == std::remove(get_shared_file_full_path(name).c_str());
    }

private:

    std::string name_;
    int fd_;

#ifdef _MSC_VER

    static int open_and_lock_file(
            const std::string& file_path,
            bool* was_lock_created)
    {
        int test_exist;
        auto ret = _sopen_s(&test_exist, file_path.c_str(), O_RDONLY, _SH_DENYRW, _S_IREAD | _S_IWRITE);

        if (ret == 0)
        {
            *was_lock_created = false;
            return test_exist;
        }

        int fd;
        ret = _sopen_s(&fd, file_path.c_str(), O_CREAT | O_RDONLY, _SH_DENYRW, _S_IREAD | _S_IWRITE);

        if (ret != 0)
        {
            return -1;
        }

        *was_lock_created = true;

        return fd;
    }

    static void unlock_and_close(
            int fd,
            const std::string& name)
    {
        _close(fd);

        if (0 != std::remove(get_shared_file_full_path(name).c_str()))
        {
        }
    }

#else

    static int open_and_lock_file(
            const std::string& file_path,
            bool* was_lock_created)
    {
        auto fd = open(file_path.c_str(), O_RDONLY, 0);

        if (fd != -1)
        {
            *was_lock_created = false;
        }
        else
        {
            *was_lock_created = true;
            fd = open(file_path.c_str(), O_CREAT | O_RDONLY, 0666);
        }

        if (fd == -1)
        {
            return -1;
        }

        // Lock the file
        if (0 != flock(fd, LOCK_EX | LOCK_NB))
        {
            close(fd);
            return -1;
        }

        return fd;
    }

    static void unlock_and_close(
            int fd,
            const std::string& name)
    {
        flock(fd, LOCK_UN | LOCK_NB);
        close(fd);

        if (0 != std::remove(get_shared_file_full_path(name).c_str()))
        {
        }
    }

#endif // ifdef _MSC_VER

};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_SYNC_ROBUST_EXCLUSIVE_LOCK_H_
