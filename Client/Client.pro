QT  += core gui
QT  += opengl               # OpenGL サポート（QOpenGLFunctionsなど）
QT  += openglwidgets        # QOpenGLWidget など
QT  += websockets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

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
}

win32 {
    LIBS += ws2_32.lib

    !isEmpty( KVS_DIR ) {
        equals( KVS_SUPPORT_QT, "1" ) {
            LIBS += -L$$KVS_DIR/lib -lkvsSupportQt -lkvsCore
        }
        equals( KVS_ENABLE_OPENGL, "1" ) {
            LIBS += -lopengl32
        }
        equals( KVS_ENABLE_GLU, "1" ) {
            LIBS += -lglu32
        }
        equals( KVS_ENABLE_GLEW, "1" ) {
            LIBS += -L$$KVS_GLEW_DIR/lib -lglew32
        }
        equals( KVS_SUPPORT_GLUT, "1" ) {
            LIBS += -L$$KVS_GLUT_DIR/lib -lfreeglut
            LIBS += -L$$KVS_DIR/lib -lkvsSupportGLUT
        }
        equals( KVS_SUPPORT_OPENXR, "1" ) {
            LIBS += -L$$KVS_DIR/lib -lkvsSupportOpenXR
            LIBS += -L$$KVS_OPENXR_DIR/lib -lopenxr_loader
            LIBS += -lgdi32
        }
    }
    !isEmpty( KVS_ASSIMP_DIR ) {
        LIBS += -L$$KVS_ASSIMP_DIR/lib/release -lassimp-vc143-mt
    }
}

macx {
    !isEmpty( KVS_DIR ) {
        equals( KVS_SUPPORT_QT, "1" ) {
            LIBS += -L$$KVS_DIR/lib -lkvsSupportQt -lkvsCore
        }
        equals( KVS_ENABLE_OPENGL, "1" ) {
            LIBS += -framework OpenGL
        }
        equals( KVS_ENABLE_GLU, "1" ) {
        }
        equals( KVS_ENABLE_GLEW, "1" ) {
        }
        equals( KVS_SUPPORT_GLUT, "1" ) {
            LIBS += -framework GLUT
            LIBS += -L$$KVS_DIR/lib -lkvsSupportGLUT
        }
    }
    !isEmpty( KVS_ASSIMP_DIR ) {
        LIBS += -L$$KVS_ASSIMP_DIR/lib -lassimp -lIrrXML -lzlibstatic
    }
}

unix:!macx {
    !isEmpty( KVS_DIR ) {
        equals( KVS_SUPPORT_QT, "1" ) {
            LIBS += -L$$KVS_DIR/lib -lkvsSupportQt -lkvsCore
        }
        equals( KVS_ENABLE_OPENGL, "1" ) {
            LIBS += -lGL
        }
        equals( KVS_ENABLE_GLU, "1" ) {
            LIBS += -lGLU
        }
        equals( KVS_ENABLE_GLEW, "1" ) {
        }
        equals( KVS_SUPPORT_GLUT, "1" ) {
            LIBS += -lglut
            LIBS += -L$$KVS_DIR/lib -lkvsSupportGLUT
        }
    }
    !isEmpty( KVS_ASSIMP_DIR ) {
        LIBS += -L$$KVS_ASSIMP_DIR/lib -lassimp -lIrrXML -lzlibstatic
    }
    LIBS += -lstdc++fs
}

SOURCES += \
    Client.cpp \
    main.cpp

HEADERS += \
    Client.h

FORMS += \
    Client.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
