#pragma once

/// Number of failures encountered while setting up functions, hooks, and globals.
static int g_Failures = 0;

static void Fail() {
    ++g_Failures;
}