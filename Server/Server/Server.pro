QT       = core
CONFIG  += c++20 cmdline

SOURCES += \
    Server.cpp \
    main.cpp

HEADERS += \
    Server.h

# ------------------------------
# デプロイ先 (QNX / Unix / etc.)
# ------------------------------
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# ------------------------------
# uWebSockets/uSockets 設定
# ------------------------------
KVS_UWS_DIR     = $$(KVS_UWS_DIR)
isEmpty(KVS_UWS_DIR) {
    message("KVS_UWS_DIR is not defined.")
} else {
    message("KVS_UWS_DIR is set to: $$KVS_UWS_DIR")

    # 共通インクルード
    INCLUDEPATH += \
        $$KVS_UWS_DIR/src \
        $$KVS_UWS_DIR/uSockets/src

    DEFINES += UWS

    # --------------------------
    # Windows 用
    # --------------------------
    win32 {
        INCLUDEPATH += $$KVS_UWS_DIR/x64-windows-static/include

        CONFIG(release, debug|release) {
            # LIBS += -L../Hoge/release -lHoge
        } else:CONFIG(debug, debug|release) {
            # LIBS += -L../Hoge/debug -lHoge
        }

        LIBS += -L$$KVS_UWS_DIR/x64-windows-static/lib \
                uSockets.lib \
                libuv.lib \
                zlib.lib

        # Windows 標準ライブラリ群
        LIBS += -ladvapi32 -luserenv -lkernel32 -luser32 -lgdi32 \
                -lws2_32 -liphlpapi -lsecur32 -lpsapi -lshell32 \
                -lole32 -loleaut32 -lshlwapi -lDbghelp -lrpcrt4
    }

    # --------------------------
    # macOS 用
    # --------------------------
    macx {
        LIBS += $$KVS_UWS_DIR/uSockets/uSockets.a

        OPENSSL_PATH = /opt/homebrew/opt/openssl@3
        LIBS += -L$$OPENSSL_PATH/lib -lssl -lcrypto -lz
    }

    # --------------------------
    # Linux (mac 以外の unix)
    # --------------------------
    unix:!macx {
        LIBS += $$KVS_UWS_DIR/uSockets/uSockets.a
        LIBS += -lssl -lcrypto -lz -lpthread
    }
}
