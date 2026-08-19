#include <array>
#include <vector>

#include <mutex>
#include <atomic>

#include <pybind11/pybind11.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Wininet.h>

#include <ras.h>
#include <raserror.h>

#if !defined(UNICODE) || !defined(_UNICODE)
    #error "Define unicode platform instead"
#endif

namespace py = pybind11;

namespace {
    using atomic_window_t = std::atomic<void *>;

    std::mutex Mutex;

    struct DaemonState {
        atomic_window_t window;
        std::mutex window_mutex;

        DaemonState() : window(nullptr) {}

        ~DaemonState()
        {
            const auto handle = static_cast<HWND>(window.load());
            if (handle) {
                PostMessage(handle, WM_CLOSE, 0, 0);
            }
        }
    };

    DaemonState *get_daemon_state(HWND window)
    {
        return reinterpret_cast<DaemonState *>(
            GetWindowLongPtr(window, GWLP_USERDATA));
    }

    bool apply_connect(INTERNET_PER_CONN_OPTION_LIST *option, LPTSTR conn)
    {
        option->pszConnection = conn;

        if (!InternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, option, sizeof(*option)) ||
            !InternetSetOption(NULL, INTERNET_OPTION_PROXY_SETTINGS_CHANGED, NULL, 0) ||
            !InternetSetOption(NULL, INTERNET_OPTION_REFRESH, NULL, 0)) {
            return false;
        }

        return true;
    }

    bool apply(INTERNET_PER_CONN_OPTION_LIST *option_list)
    {
        DWORD dwCb = 0, dwEntries = 0;

        if (RasEnumEntries(NULL, NULL, NULL, &dwCb, &dwEntries) == ERROR_BUFFER_TOO_SMALL) {
            std::vector<RASENTRYNAME> RasEntryName(dwEntries);

            for (auto& ras_entry : RasEntryName) {
                ras_entry.dwSize = static_cast<DWORD>(sizeof(RASENTRYNAME));
            }

            if (RasEnumEntries(NULL, NULL, RasEntryName.data(), &dwCb, &dwEntries) != ERROR_SUCCESS) {
                return false;
            }

            // Default/LAN connection
            if (!apply_connect(option_list, NULL)) {
                return false;
            }

            for (auto& ras_entry : RasEntryName) {
                if (!apply_connect(option_list, ras_entry.szEntryName)) {
                    return false;
                }
            }

            return true;
        }

        if (dwEntries >= 1) {
            return false;
        }

        // No ras entry. Set Default/LAN connection only.
        return apply_connect(option_list, NULL);
    }

    bool off()
    {
        std::lock_guard<std::mutex> lock(Mutex);

        std::array<INTERNET_PER_CONN_OPTION, 1> options{};
        INTERNET_PER_CONN_OPTION_LIST option_list{};

        options[0].dwOption = INTERNET_PER_CONN_FLAGS;
        options[0].Value.dwValue = PROXY_TYPE_AUTO_DETECT | PROXY_TYPE_DIRECT;

        option_list.dwSize = static_cast<DWORD>(sizeof(option_list));
        option_list.dwOptionCount = static_cast<DWORD>(options.size());
        option_list.dwOptionError = 0;
        option_list.pOptions = options.data();

        return apply(&option_list);
    }

    bool pac(LPTSTR pac_url)
    {
        std::lock_guard<std::mutex> lock(Mutex);

        std::array<INTERNET_PER_CONN_OPTION, 2> options{};
        INTERNET_PER_CONN_OPTION_LIST option_list{};

        options[0].dwOption = INTERNET_PER_CONN_FLAGS;
        options[0].Value.dwValue = PROXY_TYPE_AUTO_PROXY_URL | PROXY_TYPE_DIRECT;

        options[1].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
        options[1].Value.pszValue = pac_url;

        option_list.dwSize = static_cast<DWORD>(sizeof(option_list));
        option_list.dwOptionCount = static_cast<DWORD>(options.size());
        option_list.dwOptionError = 0;
        option_list.pOptions = options.data();

        return apply(&option_list);
    }

    bool set(LPTSTR server, LPTSTR bypass)
    {
        std::lock_guard<std::mutex> lock(Mutex);

        std::array<INTERNET_PER_CONN_OPTION, 3> options{};
        INTERNET_PER_CONN_OPTION_LIST option_list{};

        options[0].dwOption = INTERNET_PER_CONN_FLAGS;
        options[0].Value.dwValue = PROXY_TYPE_PROXY | PROXY_TYPE_DIRECT;

        options[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
        options[1].Value.pszValue = server;

        options[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
        options[2].Value.pszValue = bypass;

        option_list.dwSize = static_cast<DWORD>(sizeof(option_list));
        option_list.dwOptionCount = static_cast<DWORD>(options.size());
        option_list.dwOptionError = 0;
        option_list.pOptions = options.data();

        return apply(&option_list);
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message) {
        case WM_NCCREATE: {
            const auto create = reinterpret_cast<CREATESTRUCT *>(lParam);
            SetWindowLongPtr(
                hWnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY: {
            auto state = get_daemon_state(hWnd);
            if (state) {
                void *expected = hWnd;
                state->window.compare_exchange_strong(expected, nullptr);
            }
            PostQuitMessage(0U);
            break;
        }
        case WM_QUERYENDSESSION:
            /*
             * Windows requires a prompt response to this message. Apply the
             * process-wide proxy cleanup directly; no Python state is touched.
             */
            off();
            return TRUE;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        return 0;
    }

    bool daemon_off(DaemonState& state)
    {
        std::lock_guard<std::mutex> lock(state.window_mutex);

        const auto window = static_cast<HWND>(state.window.load());
        if (window) {
            return static_cast<bool>(PostMessage(window, WM_CLOSE, 0, 0));
        }

        // no daemon running
        return true;
    }

    bool daemon_on_(DaemonState& state)
    {
        HWND window = nullptr;

        {
            std::lock_guard<std::mutex> lock(state.window_mutex);

            if (state.window.load()) {
                // daemon already started
                return true;
            }

            WNDCLASSEX wx{};

            wx.cbSize        = sizeof(wx);
            wx.lpfnWndProc   = WndProc;
            wx.lpszClassName = L"sysproxy_class";

            if (!RegisterClassEx(&wx) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                return false;
            }

            window = CreateWindowEx(
                0,
                wx.lpszClassName,
                L"sysproxy",
                0,
                0,
                0,
                0,
                0,
                nullptr,
                nullptr,
                nullptr,
                &state);

            if (!window) {
                return false;
            }

            state.window.store(window);
        }

        bool success = true;
        {
            py::gil_scoped_release release;

            MSG msg{};
            BOOL result = 0;

            while ((result = GetMessage(&msg, nullptr, 0, 0)) > 0) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            success = result != -1;
        }

        // GetMessage can also end because another component posted WM_QUIT or
        // because of an error. Do not leave a live HWND pointing at this state.
        if (IsWindow(window)) {
            DestroyWindow(window);
        }

        void *expected = window;
        state.window.compare_exchange_strong(expected, nullptr);

        return success;
    }

#if PYBIND11_VERSION_HEX >= 0x03000000
    PYBIND11_MODULE(
        sysproxy,
        m,
        py::mod_gil_not_used(),
        py::multiple_interpreters::per_interpreter_gil()) {
#elif PYBIND11_VERSION_HEX >= 0x020D0000
    PYBIND11_MODULE(sysproxy, m, py::mod_gil_not_used()) {
#else
    PYBIND11_MODULE(sysproxy, m) {
#endif
        auto state = py::capsule(
            new DaemonState(),
            "sysproxy.daemon_state",
            [](void *value) {
                delete static_cast<DaemonState *>(value);
            });

        m.attr("_daemon_state") = state;

        m.def("off", &off,
            "Turn proxy settings off");

        m.def("pac", &pac,
            "Turn proxy settings on with PAC",
            py::arg("pac_url"));

        m.def("set", &set,
            "Turn proxy settings on with server and bypass",
            py::arg("server"), py::arg("bypass"));

        m.def("daemon_off", [state]() {
                return daemon_off(
                    *static_cast<DaemonState *>(state.get_pointer()));
            },
            "Turn proxy daemon off");

        m.def("daemon_on_", [state]() {
                return daemon_on_(
                    *static_cast<DaemonState *>(state.get_pointer()));
            },
            "Turn proxy daemon on");
    }
}
