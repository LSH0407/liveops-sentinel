enable_testing()

add_executable(unit_tests
  test_main.cpp
  test_metrics.cpp
  test_probe.cpp
  test_config.cpp
  test_recommendation.cpp
  test_eventlog.cpp
  test_reportwriter.cpp
  test_thresholds.cpp
  test_alert_cooldown.cpp
)

target_include_directories(unit_tests PRIVATE ../src)
target_link_libraries(unit_tests PRIVATE doctest::doctest)

add_test(NAME UnitTests COMMAND unit_tests)
