#ifndef FUTURE_SCHEDULER_H
#define FUTURE_SCHEDULER_H

#include <atomic>
#include <functional>

#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QJSValue>
#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QWaitCondition>
#include <QDebug>

class FutureScheduler : public QObject
{
    Q_OBJECT

public:
    FutureScheduler(QObject *parent);
    ~FutureScheduler();

    void shutdownWaitForFinished() noexcept;

    QPair<bool, QFuture<void>> run(std::function<void()> function) noexcept;
    QPair<bool, QFuture<QJSValueList>> run(std::function<QJSValueList() noexcept> function, const QJSValue &callback);

private:
    size_t add() noexcept;
    void done() noexcept;

    template<typename T>
    QFutureWatcher<T> *newWatcher()
    {
        QFutureWatcher<T> *watcher = new QFutureWatcher<T>();
        QThread *schedulerThread = this->thread();
        if (watcher->thread() != schedulerThread)
        {
            watcher->moveToThread(schedulerThread);
        }
        watcher->setParent(this);

        return watcher;
    }

    template<typename T>
    QPair<bool, QFuture<T>> execute(std::function<QFuture<T>(QFutureWatcher<T> *, size_t)> makeFuture) noexcept
    {
        size_t id = add();
        if (id != 0)
        {
            try
            {
                auto *watcher = newWatcher<T>();
                watcher->setFuture(makeFuture(watcher, id));
                connect(watcher, &QFutureWatcher<T>::finished, [this, watcher] {
                    watcher->deleteLater();
                });
                return qMakePair(true, watcher->future());
            }
            catch (const std::exception &exception)
            {
                qCritical() << "Failed to schedule async function: " << exception.what();
                done();
            }
        }

        return qMakePair(false, QFuture<T>());
    }

    QFutureWatcher<void> schedule(std::function<void()> function);
    QFutureWatcher<QJSValueList> schedule(std::function<QJSValueList() noexcept> function, const QJSValue &callback);

private:
    size_t Alive;
    QWaitCondition Condition;
    QMutex Mutex;
    bool Stopping;
    std::atomic<std::size_t> Id;
};

#endif // FUTURE_SCHEDULER_H
