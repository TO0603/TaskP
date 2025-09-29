QT       = core
CONFIG  += c++20 cmdline

# macOSで std::filesystem を有効にする
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
}

# ビルドタイプ別の最適化フラグ
CONFIG( release, debug|release ) {
    CONFIG += release

    win32 {
        QMAKE_CXXFLAGS_RELEASE -= -O2
        QMAKE_CXXFLAGS_RELEASE += /Ox
        QMAKE_CFLAGS_RELEASE += /MT
        QMAKE_CXXFLAGS_RELEASE += /MT
    }

    macx {
        QMAKE_CXXFLAGS_RELEASE -= -O2
        QMAKE_CXXFLAGS_RELEASE += -O3
    }

    unix:!macx {
        QMAKE_CXXFLAGS_RELEASE -= -O2
        QMAKE_CXXFLAGS_RELEASE += -O3
    }
}
else:CONFIG( debug, debug|release ) {
    CONFIG += debug

    win32 {
        QMAKE_CXXFLAGS_DEBUG += /Od /DDEBUG
        QMAKE_CFLAGS_DEBUG += /MT
        QMAKE_CXXFLAGS_DEBUG += /MT
    }

    macx {
        QMAKE_CXXFLAGS_DEBUG += -O0 -g -DDEBUG
    }

    unix:!macx {
        QMAKE_CXXFLAGS_DEBUG += -O0 -g -DDEBUG
    }
}

KVS_DIR         = $$(KVS_DIR)
KVS_GLEW_DIR    = $$(KVS_GLEW_DIR)
KVS_GLUT_DIR    = $$(KVS_GLUT_DIR)
KVS_OPENXR_DIR  = $$(KVS_OPENXR_DIR)
KVS_IMGUI_DIR   = $$(KVS_IMGUI_DIR)
KVS_ASSIMP_DIR  = $$(KVS_ASSIMP_DIR)
KVS_UWS_DIR     = $$(KVS_UWS_DIR)

isEmpty( KVS_DIR ) {
    error( "The environment variable KVS_DIR is not defined." )
}
else {
    include( $$KVS_DIR/kvs.conf )
    win32 {
        DEFINES += WIN32 _MBCS NOMINMAX _SCL_SECURE_NO_DEPRECATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE
    }
    !isEmpty( DEBUG ) {
        DEFINES += _DEBUG KVS_ENABLE_DEBUG
    } else {
        DEFINES += NDEBUG
    }
    INCLUDEPATH += $$KVS_DIR/include

    equals( KVS_ENABLE_OPENGL, "1" ) {  # すべてのプラットフォームで必要
        message( "KVS_ENABLE_OPENGL" )
        DEFINES += KVS_ENABLE_OPENGL
    }

    equals( KVS_ENABLE_GLU, "1" ){      # すべてのプラットフォームで必要
        message( "KVS_ENABLE_GLU" )
        DEFINES += KVS_ENABLE_GLU
    }

    equals( KVS_ENABLE_GLEW, "1" ){     # Windowsのみ必要
        message( "KVS_ENABLE_GLEW" )
        DEFINES += KVS_ENABLE_GLEW
        INCLUDEPATH += $$KVS_GLEW_DIR/include
    }

    equals( KVS_SUPPORT_GLUT, "1" ){    # すべてのプラットフォームで必要(?)
        message( "KVS_SUPPORT_GLUT" )
        DEFINES += KVS_SUPPORT_GLUT
        INCLUDEPATH += $$KVS_GLUT_DIR/include
    }

    equals( KVS_SUPPORT_QT, "1" ){      # すべてのプラットフォームで必要
        message( "KVS_SUPPORT_QT" )
        DEFINES += KVS_SUPPORT_QT
    }

    equals( KVS_SUPPORT_OPENXR, "1" ){  # Windows専用（オプション機能）
        message( "KVS_SUPPORT_OPENXR" )
        isEmpty( KVS_IMGUI_DIR ){
            error( "To use the OPENXR feature, you must configure IMGUI." )
        }
        else{
            win32 {
                INCLUDEPATH += $$KVS_OPENXR_DIR/include
                INCLUDEPATH += $$KVS_IMGUI_DIR/include
                DEFINES += KVS_SUPPORT_OPENXR
                # DEFINES += OPENXR_SCREEN # オプション
            }
        }
    }

    isEmpty( KVS_ASSIMP_DIR ){          # 任意（全プラットフォーム対応）
        message( "KVS_ASSIMP_DIR is not defined." )
    }
    else{
        message( "KVS_ASSIMP_DIR is set to: $$KVS_ASSIMP_DIR" )
        INCLUDEPATH += $$KVS_ASSIMP_DIR/include
        DEFINES += ASSIMP
    }

    isEmpty( KVS_UWS_DIR ){             # サーバ側必須(全プラットフォーム対応)
        message( "KVS_UWS_DIR is not defined." )
    }
    else{
        win32 {
            message( "KVS_UWS_DIR is set to: $$KVS_UWS_DIR" )
            INCLUDEPATH += $$KVS_UWS_DIR/x64-windows-static/include
        }
        else{
            message( "KVS_UWS_DIR is set to: $$KVS_UWS_DIR" )
            INCLUDEPATH += $$KVS_UWS_DIR/src
            INCLUDEPATH += $$KVS_UWS_DIR/uSockets/src
            DEFINES += UWS
        }
    }
}

win32 {
    CONFIG(release, debug|release){
        # LIBS += -L../Hoge/release -lHoge
    }
    else:CONFIG(debug, debug|release){
        # LIBS += -L../Hoge/debug -lHoge
    }

    !isEmpty( KVS_DIR ) {
        LIBS += -L$$KVS_DIR/lib -lkvsCore
        equals( KVS_ENABLE_OPENGL, "1" ) {
            LIBS += -lopengl32
        }
        equals( KVS_ENABLE_GLU, "1" ) {
            LIBS += -lglu32
        }
        equals( KVS_ENABLE_GLEW, "1" ) {
            LIBS += -L$$KVS_GLEW_DIR/lib -lglew32
        }
    }
    !isEmpty( KVS_UWS_DIR ) {
        LIBS += -L$$KVS_UWS_DIR/x64-windows-static/lib \
            uSockets.lib \
            libuv.lib \
            zlib.lib

        LIBS += -ladvapi32 \
            -luserenv \
            -lkernel32 \
            -luser32 \
            -lgdi32 \
            -lws2_32 \
            -liphlpapi \
            -lsecur32 \
            -lpsapi \
            -lshell32 \
            -lole32 \
            -loleaut32 \
            -lshlwapi \
            -lDbghelp \
            -lrpcrt4
    }
}

macx {
    !isEmpty( KVS_DIR ) {
        LIBS += -L$$KVS_DIR/lib -lkvsCore
    }
    !isEmpty( KVS_UWS_DIR ) {
        LIBS += $$KVS_UWS_DIR/uSockets/uSockets.a
        OPENSSL_PATH = /opt/homebrew/opt/openssl@3
        LIBS += -L$$OPENSSL_PATH/lib -lssl -lcrypto -lz
    }
}

unix:!macx {
    !isEmpty( KVS_DIR ) {
        LIBS += -L$$KVS_DIR/lib -lkvsCore
    }
    !isEmpty( KVS_UWS_DIR ) {
        LIBS += $$KVS_UWS_DIR/uSockets/uSockets.a
        LIBS += -lssl -lcrypto -lz
    }
}

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
