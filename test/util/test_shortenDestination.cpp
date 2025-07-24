#include <Arduino.h>
#include <unity.h>
#include "util.h"

void test_shortenDestination_basic()
{
    String dep = "Frankfurt Hauptbahnhof";
    String dest = "Frankfurt Flughafen";
    String expected = "Flughafen";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), shortenDestination(dep, dest).c_str());
}

void test_shortenDestination_no_common_prefix()
{
    String dep = "Mainz";
    String dest = "Frankfurt Hauptbahnhof";
    String expected = "Frankfurt Hauptbahnhof";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), shortenDestination(dep, dest).c_str());
}

void test_shortenDestination_partial_overlap()
{
    String dep = "Frankfurt (Main) Rödelheim Bf";
    String dest = "Frankfurt (Main) Hauptbahnhof";
    String expected = "Hauptbahnhof";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), shortenDestination(dep, dest).c_str());
}

void test_shortenDestination_empty_destination()
{
    String dep = "Frankfurt";
    String dest = "";
    String expected = "";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), shortenDestination(dep, dest).c_str());
}

void test()
{
    setup();
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_shortenDestination_basic);
    RUN_TEST(test_shortenDestination_no_common_prefix);
    RUN_TEST(test_shortenDestination_partial_overlap);
    RUN_TEST(test_shortenDestination_empty_destination);
    UNITY_END();
}

void loop()
{
    // not used
}

