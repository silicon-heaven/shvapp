#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/leafnode.h"
#include "src/historyapp.h"
#include "tests/sites.h"
#include "tests/utils.h"

#include <shv/core/utils/shvlogrpcvaluereader.h>
using Severity = shv::core::utils::ShvAlarm::Severity;

shv::core::utils::ShvAlarm make_alarm(const std::string& path, shv::core::utils::ShvAlarm::Severity severity = shv::core::utils::ShvAlarm::Severity::Error)
{
	return shv::core::utils::ShvAlarm(path, true, severity, 0, "Alarm 1", "Alarm 1 label");
}

QQueue<std::function<CallNext(MockRpcConnection*)>> setup_test()
{
	QQueue<std::function<CallNext(MockRpcConnection*)>> res;

	DOCTEST_SUBCASE("no typeinfo")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			SEND_SITES_YIELD(mock_sites::some_site);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			DISABLE_TYPEINFO("some_site");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv/some_site", "chng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv/some_site", "cmdlog");
		});
	}

	DOCTEST_SUBCASE("with typeinfo")
	{
		DOCTEST_SUBCASE("one device, one alarm") {
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_SITES_YIELD(mock_sites::some_site);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				ENABLE_TYPEINFO("some_site");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv/some_site", "chng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION("shv/some_site", "cmdlog");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_TYPEINFO("some_site", mock_typeinfo::one_device);
				REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				return CallNext::Yes;
			});

			DOCTEST_SUBCASE("alarm on") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/one/status", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status/some_alarm_name"),
					});
				});
			}

			DOCTEST_SUBCASE("alarm off") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY("shv/some_site/one/status", "chng", 0);
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}

			DOCTEST_SUBCASE("alarm on and off") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/one/status", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status/some_alarm_name"),
					});

					NOTIFY_YIELD("shv/some_site/one/status", "chng", 0);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Invalid));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}
		}
		DOCTEST_SUBCASE("one device, two alarms") {
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_SITES_YIELD(mock_sites::some_site);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				ENABLE_TYPEINFO("some_site");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv/some_site", "chng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION("shv/some_site", "cmdlog");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_TYPEINFO("some_site", mock_typeinfo::two_alarms);
				REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				return CallNext::Yes;
			});

			DOCTEST_SUBCASE("alarm 1 on") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/one/status1", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status1/some_alarm_name"),
					});
				});
			}

			DOCTEST_SUBCASE("alarm 1 off") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY("shv/some_site/one/status1", "chng", 0);
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}

			DOCTEST_SUBCASE("alarm 1 on and off") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/one/status1", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status1/some_alarm_name"),
					});

					NOTIFY_YIELD("shv/some_site/one/status1", "chng", 0);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Invalid));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}

			DOCTEST_SUBCASE("alarm 2 on") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/one/status2", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status2/some_alarm_name"),
					});
				});
			}

			DOCTEST_SUBCASE("alarm 2 off") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY("shv/some_site/one/status2", "chng", 0);
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}

			DOCTEST_SUBCASE("alarm 2 on and off") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/one/status2", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status2/some_alarm_name"),
					});

					NOTIFY_YIELD("shv/some_site/one/status2", "chng", 0);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Invalid));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}
			DOCTEST_SUBCASE("both alarm 1 and 2 on and then 1 off") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/one/status1", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status1/some_alarm_name"),
					});
					NOTIFY_YIELD("shv/some_site/one/status2", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status1/some_alarm_name"),
						make_alarm("one/status2/some_alarm_name"),
					});
					NOTIFY_YIELD("shv/some_site/one/status1", "chng", 0);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status2/some_alarm_name"),
					});
				});
			}
		}

		DOCTEST_SUBCASE("two devices, each has one alarm") {
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_SITES_YIELD(mock_sites::some_site);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				ENABLE_TYPEINFO("some_site");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv/some_site", "chng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION("shv/some_site", "cmdlog");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_TYPEINFO("some_site", mock_typeinfo::two_device);
				REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				return CallNext::Yes;
			});

			DOCTEST_SUBCASE("alarm on device 1") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/one/status", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status/some_alarm_name"),
					});
				});
			}

			DOCTEST_SUBCASE("alarm off device 1") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY("shv/some_site/one/status", "chng", 0);
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}

			DOCTEST_SUBCASE("alarm on and off 1") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/one/status", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("one/status/some_alarm_name"),
					});

					NOTIFY_YIELD("shv/some_site/one/status", "chng", 0);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Invalid));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}

			DOCTEST_SUBCASE("alarm on device 2") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/two/status", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm("two/status/some_alarm_name"),
					});
				});
			}

			DOCTEST_SUBCASE("alarm off device 2") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY("shv/some_site/two/status", "chng", 0);
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}

			DOCTEST_SUBCASE("alarm on and off 2") {
				enqueue(res, [=] (MockRpcConnection* mock) {
					NOTIFY_YIELD("shv/some_site/two/status", "chng", 1);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
						make_alarm( "two/status/some_alarm_name")
					});

					NOTIFY_YIELD("shv/some_site/two/status", "chng", 0);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site", "alarmchng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Invalid));
					REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				});
			}
		}

		DOCTEST_SUBCASE("severity sorting") {
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_SITES_YIELD(mock_sites::some_site);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				ENABLE_TYPEINFO("some_site");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv/some_site", "chng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION("shv/some_site", "cmdlog");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_TYPEINFO("some_site", mock_typeinfo::different_severity);
				REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{});
				NOTIFY_YIELD("shv/some_site/one/status1", "chng", 1);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SIGNAL("some_site", "alarmchng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Warning));
				REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
					make_alarm("one/status1/alarm_warning", shv::core::utils::ShvAlarm::Severity::Warning),
				});
				NOTIFY_YIELD("shv/some_site/one/status2", "chng", 1);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SIGNAL("some_site", "alarmchng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SIGNAL("some_site:overallAlarm", "chng", static_cast<int>(Severity::Error));
				REQUIRE(HistoryApp::instance()->leafNode("some_site")->alarms() == std::vector<shv::core::utils::ShvAlarm>{
					make_alarm("one/status2/alarm_error"),
					make_alarm("one/status1/alarm_warning", shv::core::utils::ShvAlarm::Severity::Warning),
				});
			});
		}
	}
	return res;
}

TEST_HISTORYPROVIDER_MAIN("alarms")
