#include <dispatcher/api/transport_adapter.hpp>
#include <dispatcher/api/transport_adapter_options.hpp>
#include <dispatcher/api/transport_adapter_registry.hpp>
#include <dispatcher/api/transport_adapter_result.hpp>
#include <dispatcher/api/transport_adapter_status.hpp>
#include <dispatcher/api/transport_protocol.hpp>
#include <dispatcher/api/transport_router.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

namespace
{
    class FakeTransportAdapter final : public dispatcher::api::TransportAdapter
    {
    public:
        explicit FakeTransportAdapter(
            dispatcher::api::TransportAdapterOptions options,
            std::string adapter_name = "fake adapter"
        )
            : options_(std::move(options))
            , name_(std::move(adapter_name))
        {
            if (options_.disabled())
            {
                status_ = dispatcher::api::TransportAdapterStatus::Disabled;
            }
        }

        [[nodiscard]] dispatcher::api::TransportProtocol protocol()
            const noexcept override
        {
            return options_.protocol();
        }

        [[nodiscard]] std::string name() const override
        {
            return name_;
        }

        [[nodiscard]] dispatcher::api::TransportAdapterStatus status()
            const noexcept override
        {
            return status_;
        }

        [[nodiscard]] const dispatcher::api::TransportAdapterOptions& options()
            const noexcept override
        {
            return options_;
        }

        void set_router(
            std::shared_ptr<dispatcher::api::TransportRouter> router
        ) override
        {
            router_ = std::move(router);
        }

        [[nodiscard]] bool has_router() const noexcept override
        {
            return router_ != nullptr;
        }

        [[nodiscard]] dispatcher::api::TransportAdapterResult start() override
        {
            if (options_.disabled())
            {
                status_ = dispatcher::api::TransportAdapterStatus::Disabled;

                return dispatcher::api::TransportAdapterResult::failure(
                    protocol(),
                    dispatcher::api::TransportAdapterStatus::Failed,
                    "transport adapter is disabled",
                    name_
                );
            }

            if (!options_.valid())
            {
                status_ = dispatcher::api::TransportAdapterStatus::Failed;

                return dispatcher::api::TransportAdapterResult::failure(
                    protocol(),
                    dispatcher::api::TransportAdapterStatus::Failed,
                    "transport adapter options are invalid",
                    name_
                );
            }

            if (!has_router())
            {
                status_ = dispatcher::api::TransportAdapterStatus::Failed;

                return dispatcher::api::TransportAdapterResult::failure(
                    protocol(),
                    dispatcher::api::TransportAdapterStatus::Failed,
                    "transport adapter router is missing",
                    name_
                );
            }

            status_ = dispatcher::api::TransportAdapterStatus::Running;

            return dispatcher::api::TransportAdapterResult::success(
                protocol(),
                status_,
                "transport adapter started"
            );
        }

        [[nodiscard]] dispatcher::api::TransportAdapterResult stop() override
        {
            if (status_ == dispatcher::api::TransportAdapterStatus::Disabled)
            {
                return dispatcher::api::TransportAdapterResult::success(
                    protocol(),
                    status_,
                    "transport adapter disabled"
                );
            }

            status_ = dispatcher::api::TransportAdapterStatus::Stopped;

            return dispatcher::api::TransportAdapterResult::success(
                protocol(),
                status_,
                "transport adapter stopped"
            );
        }

        [[nodiscard]] dispatcher::api::TransportAdapterResult health()
            const override
        {
            if (running())
            {
                return dispatcher::api::TransportAdapterResult::success(
                    protocol(),
                    status_,
                    "transport adapter healthy"
                );
            }

            return dispatcher::api::TransportAdapterResult::failure(
                protocol(),
                dispatcher::api::TransportAdapterStatus::Failed,
                "transport adapter is not running",
                name_
            );
        }

    private:
        dispatcher::api::TransportAdapterOptions options_;
        std::string name_;
        dispatcher::api::TransportAdapterStatus status_{
            dispatcher::api::TransportAdapterStatus::Stopped
        };
        std::shared_ptr<dispatcher::api::TransportRouter> router_;
    };

    std::shared_ptr<FakeTransportAdapter> make_fake_adapter(
        dispatcher::api::TransportProtocol protocol =
        dispatcher::api::TransportProtocol::Http,
        std::uint16_t port = 8080,
        bool enabled = true
    )
    {
        return std::make_shared<FakeTransportAdapter>(
            dispatcher::api::TransportAdapterOptions(
                protocol,
                "127.0.0.1",
                port,
                enabled
            )
        );
    }
}

TEST(TransportAdapterStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportAdapterStatus::Stopped
        ),
        "stopped"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportAdapterStatus::Starting
        ),
        "starting"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportAdapterStatus::Running
        ),
        "running"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportAdapterStatus::Stopping
        ),
        "stopping"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportAdapterStatus::Failed
        ),
        "failed"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportAdapterStatus::Disabled
        ),
        "disabled"
    );
}

TEST(TransportAdapterStatusTests, PredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::api::is_running(
            dispatcher::api::TransportAdapterStatus::Running
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_stopped(
            dispatcher::api::TransportAdapterStatus::Stopped
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_stopped(
            dispatcher::api::TransportAdapterStatus::Disabled
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_transitioning(
            dispatcher::api::TransportAdapterStatus::Starting
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_transitioning(
            dispatcher::api::TransportAdapterStatus::Stopping
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_failure(
            dispatcher::api::TransportAdapterStatus::Failed
        )
    );

    EXPECT_TRUE(
        dispatcher::api::accepts_requests(
            dispatcher::api::TransportAdapterStatus::Running
        )
    );

    EXPECT_FALSE(
        dispatcher::api::accepts_requests(
            dispatcher::api::TransportAdapterStatus::Stopped
        )
    );
}

TEST(TransportAdapterResultTests, SuccessResultWorks)
{
    const auto result =
        dispatcher::api::TransportAdapterResult::success(
            dispatcher::api::TransportProtocol::Http,
            dispatcher::api::TransportAdapterStatus::Running,
            "started"
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.protocol(),
        dispatcher::api::TransportProtocol::Http
    );

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportAdapterStatus::Running
    );

    EXPECT_TRUE(result.has_message());
    EXPECT_EQ(result.message(), "started");

    EXPECT_FALSE(result.has_detail());
}

TEST(TransportAdapterResultTests, FailureResultWorks)
{
    const auto result =
        dispatcher::api::TransportAdapterResult::failure(
            dispatcher::api::TransportProtocol::Grpc,
            dispatcher::api::TransportAdapterStatus::Failed,
            "start failed",
            "port unavailable"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.protocol(),
        dispatcher::api::TransportProtocol::Grpc
    );

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportAdapterStatus::Failed
    );

    EXPECT_TRUE(result.has_message());
    EXPECT_TRUE(result.has_detail());

    EXPECT_EQ(result.detail(), "port unavailable");
}

TEST(TransportAdapterOptionsTests, OptionsCaptureFields)
{
    const dispatcher::api::TransportAdapterOptions options(
        dispatcher::api::TransportProtocol::Http,
        "0.0.0.0",
        8080,
        true,
        std::chrono::milliseconds{ 5000 }
    );

    EXPECT_EQ(
        options.protocol(),
        dispatcher::api::TransportProtocol::Http
    );

    EXPECT_EQ(options.bind_address(), "0.0.0.0");
    EXPECT_EQ(options.port(), 8080);

    EXPECT_TRUE(options.enabled());
    EXPECT_FALSE(options.disabled());

    EXPECT_EQ(
        options.request_timeout(),
        std::chrono::milliseconds{ 5000 }
    );

    EXPECT_TRUE(options.has_bind_address());
    EXPECT_TRUE(options.has_port());

    EXPECT_TRUE(options.valid());
}

TEST(TransportAdapterOptionsTests, InvalidOptionsAreRejected)
{
    EXPECT_FALSE(
        dispatcher::api::TransportAdapterOptions(
            dispatcher::api::TransportProtocol::Unknown,
            "127.0.0.1",
            8080
        ).valid()
    );

    EXPECT_FALSE(
        dispatcher::api::TransportAdapterOptions(
            dispatcher::api::TransportProtocol::Http,
            "",
            8080
        ).valid()
    );

    EXPECT_FALSE(
        dispatcher::api::TransportAdapterOptions(
            dispatcher::api::TransportProtocol::Http,
            "127.0.0.1",
            8080,
            false
        ).valid()
    );

    EXPECT_FALSE(
        dispatcher::api::TransportAdapterOptions(
            dispatcher::api::TransportProtocol::Http,
            "127.0.0.1",
            8080,
            true,
            std::chrono::milliseconds{ 0 }
        ).valid()
    );
}

TEST(TransportAdapterTests, FakeAdapterStartsWithRouter)
{
    auto adapter = make_fake_adapter();

    EXPECT_FALSE(adapter->running());
    EXPECT_TRUE(adapter->stopped());
    EXPECT_FALSE(adapter->accepts_requests());

    adapter->set_router(
        std::make_shared<dispatcher::api::TransportRouter>()
    );

    ASSERT_TRUE(adapter->has_router());

    const auto result = adapter->start();

    EXPECT_TRUE(result.ok());

    EXPECT_TRUE(adapter->running());
    EXPECT_FALSE(adapter->stopped());
    EXPECT_TRUE(adapter->accepts_requests());

    EXPECT_EQ(
        adapter->status(),
        dispatcher::api::TransportAdapterStatus::Running
    );

    const auto health = adapter->health();

    EXPECT_TRUE(health.ok());
}

TEST(TransportAdapterTests, FakeAdapterStartFailsWithoutRouter)
{
    auto adapter = make_fake_adapter();

    const auto result = adapter->start();

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        adapter->status(),
        dispatcher::api::TransportAdapterStatus::Failed
    );

    EXPECT_FALSE(adapter->accepts_requests());
}

TEST(TransportAdapterTests, FakeAdapterStopWorks)
{
    auto adapter = make_fake_adapter();

    adapter->set_router(
        std::make_shared<dispatcher::api::TransportRouter>()
    );

    ASSERT_TRUE(adapter->start().ok());

    const auto result = adapter->stop();

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        adapter->status(),
        dispatcher::api::TransportAdapterStatus::Stopped
    );

    EXPECT_TRUE(adapter->stopped());
}

TEST(TransportAdapterRegistryTests, DefaultRegistryIsEmpty)
{
    const dispatcher::api::TransportAdapterRegistry registry;

    EXPECT_TRUE(registry.empty());
    EXPECT_EQ(registry.size(), 0);

    EXPECT_FALSE(
        registry.contains(
            dispatcher::api::TransportProtocol::Http
        )
    );

    EXPECT_TRUE(registry.adapters().empty());
    EXPECT_TRUE(registry.running_adapters().empty());
}

TEST(TransportAdapterRegistryTests, AddRegistersAdapter)
{
    dispatcher::api::TransportAdapterRegistry registry;

    const auto result =
        registry.add(
            make_fake_adapter(
                dispatcher::api::TransportProtocol::Http
            )
        );

    EXPECT_TRUE(result.ok());

    EXPECT_FALSE(registry.empty());
    EXPECT_EQ(registry.size(), 1);

    EXPECT_TRUE(
        registry.contains(
            dispatcher::api::TransportProtocol::Http
        )
    );

    const auto adapter =
        registry.find(
            dispatcher::api::TransportProtocol::Http
        );

    ASSERT_TRUE(adapter != nullptr);

    EXPECT_EQ(
        adapter->protocol(),
        dispatcher::api::TransportProtocol::Http
    );
}

TEST(TransportAdapterRegistryTests, AddRejectsNullAdapter)
{
    dispatcher::api::TransportAdapterRegistry registry;

    const auto result =
        registry.add(nullptr);

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.protocol(),
        dispatcher::api::TransportProtocol::Unknown
    );

    EXPECT_TRUE(registry.empty());
}

TEST(TransportAdapterRegistryTests, AddRejectsUnknownProtocol)
{
    dispatcher::api::TransportAdapterRegistry registry;

    const auto result =
        registry.add(
            make_fake_adapter(
                dispatcher::api::TransportProtocol::Unknown
            )
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.protocol(),
        dispatcher::api::TransportProtocol::Unknown
    );

    EXPECT_TRUE(registry.empty());
}

TEST(TransportAdapterRegistryTests, AddRejectsDuplicateProtocol)
{
    dispatcher::api::TransportAdapterRegistry registry;

    ASSERT_TRUE(
        registry.add(
            make_fake_adapter(
                dispatcher::api::TransportProtocol::Http
            )
        ).ok()
    );

    const auto result =
        registry.add(
            make_fake_adapter(
                dispatcher::api::TransportProtocol::Http,
                8081
            )
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.protocol(),
        dispatcher::api::TransportProtocol::Http
    );

    EXPECT_EQ(registry.size(), 1);
}

TEST(TransportAdapterRegistryTests, RemoveDeletesAdapter)
{
    dispatcher::api::TransportAdapterRegistry registry;

    ASSERT_TRUE(
        registry.add(
            make_fake_adapter(
                dispatcher::api::TransportProtocol::Http
            )
        ).ok()
    );

    const auto result =
        registry.remove(
            dispatcher::api::TransportProtocol::Http
        );

    EXPECT_TRUE(result.ok());

    EXPECT_TRUE(registry.empty());

    EXPECT_FALSE(
        registry.contains(
            dispatcher::api::TransportProtocol::Http
        )
    );
}

TEST(TransportAdapterRegistryTests, RemoveMissingAdapterFails)
{
    dispatcher::api::TransportAdapterRegistry registry;

    const auto result =
        registry.remove(
            dispatcher::api::TransportProtocol::Grpc
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.protocol(),
        dispatcher::api::TransportProtocol::Grpc
    );
}

TEST(TransportAdapterRegistryTests, AdaptersAreReturnedSortedByProtocol)
{
    dispatcher::api::TransportAdapterRegistry registry;

    ASSERT_TRUE(
        registry.add(
            make_fake_adapter(
                dispatcher::api::TransportProtocol::Grpc,
                50051
            )
        ).ok()
    );

    ASSERT_TRUE(
        registry.add(
            make_fake_adapter(
                dispatcher::api::TransportProtocol::Http,
                8080
            )
        ).ok()
    );

    const auto adapters = registry.adapters();

    ASSERT_EQ(adapters.size(), 2);

    EXPECT_EQ(
        adapters[0]->protocol(),
        dispatcher::api::TransportProtocol::Grpc
    );

    EXPECT_EQ(
        adapters[1]->protocol(),
        dispatcher::api::TransportProtocol::Http
    );
}

TEST(TransportAdapterRegistryTests, RunningAdaptersOnlyReturnRunningAdapters)
{
    dispatcher::api::TransportAdapterRegistry registry;

    auto http_adapter =
        make_fake_adapter(
            dispatcher::api::TransportProtocol::Http,
            8080
        );

    auto grpc_adapter =
        make_fake_adapter(
            dispatcher::api::TransportProtocol::Grpc,
            50051
        );

    const auto router =
        std::make_shared<dispatcher::api::TransportRouter>();

    http_adapter->set_router(router);

    ASSERT_TRUE(http_adapter->start().ok());

    ASSERT_TRUE(registry.add(http_adapter).ok());
    ASSERT_TRUE(registry.add(grpc_adapter).ok());

    const auto running = registry.running_adapters();

    ASSERT_EQ(running.size(), 1);

    EXPECT_EQ(
        running.front()->protocol(),
        dispatcher::api::TransportProtocol::Http
    );
}

TEST(TransportAdapterRegistryTests, SetRouterForAllUpdatesAdapters)
{
    dispatcher::api::TransportAdapterRegistry registry;

    auto http_adapter =
        make_fake_adapter(
            dispatcher::api::TransportProtocol::Http,
            8080
        );

    auto grpc_adapter =
        make_fake_adapter(
            dispatcher::api::TransportProtocol::Grpc,
            50051
        );

    ASSERT_FALSE(http_adapter->has_router());
    ASSERT_FALSE(grpc_adapter->has_router());

    ASSERT_TRUE(registry.add(http_adapter).ok());
    ASSERT_TRUE(registry.add(grpc_adapter).ok());

    registry.set_router_for_all(
        std::make_shared<dispatcher::api::TransportRouter>()
    );

    EXPECT_TRUE(http_adapter->has_router());
    EXPECT_TRUE(grpc_adapter->has_router());
}

TEST(TransportAdapterRegistryTests, ClearRemovesAllAdapters)
{
    dispatcher::api::TransportAdapterRegistry registry;

    ASSERT_TRUE(
        registry.add(
            make_fake_adapter(
                dispatcher::api::TransportProtocol::Http
            )
        ).ok()
    );

    ASSERT_EQ(registry.size(), 1);

    registry.clear();

    EXPECT_TRUE(registry.empty());
    EXPECT_EQ(registry.size(), 0);
    EXPECT_TRUE(registry.adapters().empty());
}