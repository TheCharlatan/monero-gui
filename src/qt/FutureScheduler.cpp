#include "FutureScheduler.h"

FutureScheduler::FutureScheduler(QObject *parent)
    : QObject(parent), Alive(0), Stopping(false)
{
}

FutureScheduler::~FutureScheduler()
{
    shutdownWaitForFinished();
}

void FutureScheduler::shutdownWaitForFinished() noexcept
{
    QMutexLocker locker(&Mutex);

    Stopping = true;
    while (Alive > 0)
    {
        Condition.wait(&Mutex);
    }
}

QPair<bool, QFuture<void>> FutureScheduler::run(std::function<void()> function) noexcept
{
    return execute<void>([this, function](QFutureWatcher<void> *, size_t id) {
        return QtConcurrent::run([this, function, id] {
            qCritical() << "FutureScheduler::run QtConcurrent::run " << id;
            try
            {
                function();
            }
            catch (const std::exception &exception)
            {
                qWarning() << "Exception thrown from async function: " << exception.what();
            }
            done();
            qCritical() << "FutureScheduler::run done " << id;
        });
    });
}

QPair<bool, QFuture<QJSValueList>> FutureScheduler::run(std::function<QJSValueList() noexcept> function, const QJSValue &callback)
{
    if (!callback.isCallable())
    {
        throw std::runtime_error("js callback must be callable");
    }

    return execute<QJSValueList>([this, function, callback](QFutureWatcher<QJSValueList> *watcher, size_t id) {
        connect(watcher, &QFutureWatcher<QJSValueList>::finished, [watcher, callback, id] {
            qCritical() << "invoking callback " << id;
            QJSValue(callback).call(watcher->future().result());
            qCritical() << "invoking callback done " << id;
        });
        return QtConcurrent::run([this, function, id] {
            qCritical() << "FutureScheduler::run callback QtConcurrent::run " << id;
            QJSValueList result;
            try
            {
                result = function();
            }
            catch (const std::exception &exception)
            {
                qWarning() << "Exception thrown from async function: " << exception.what();
            }
            done();
            qCritical() << "FutureScheduler::run callback done " << id;
            return result;
        });
    });
}

size_t FutureScheduler::add() noexcept
{
    QMutexLocker locker(&Mutex);

    Id++;

    if (Stopping)
    {
        return 0;
    }

    ++Alive;
    return Id;
}

void FutureScheduler::done() noexcept
{
    {
        QMutexLocker locker(&Mutex);
        --Alive;
    }

    Condition.wakeAll();
}
