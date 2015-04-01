# Copyright 2012 the V8 project authors. All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Google Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Definitions for building standalone V8 binaries to run on Android.
# This is mostly excerpted from:
# http://src.chromium.org/viewvc/chrome/trunk/src/build/common.gypi

{
  'variables': {
    # Location of Android NDK.
    'variables': {
      'android_ndk_root%': '<!(/bin/echo -n $ANDROID_NDK_ROOT)',
      'android_toolchain%': '<!(/bin/echo -n $ANDROID_TOOLCHAIN)',
    },
    'conditions': [
      ['android_ndk_root==""', {
        'variables': {
          'android_ndk_root_': '<(android_ndk_root)',
          'android_sysroot': '<(android_toolchain)/sysroot',
          'android_llvmcxx': '<(android_toolchain)/sources/cxx-stl/llvm-libc++',
        },
        'android_ndk_root': '<(android_ndk_root_)',
        'android_sysroot': '<(android_sysroot)',
        'android_include': '<(android_sysroot)/usr/include',
        'conditions': [
          ['target_arch=="x64"', {
            'android_lib': '<(android_sysroot)/usr/lib64',
          }, {
            'android_lib': '<(android_sysroot)/usr/lib',
          }],
        ],
        'android_llvmcxx_include': '<(android_llvmcxx)/llvmcxx/include',
        'android_llvmcxx_libs': '<(android_llvmcxx)/libs',
      }, {
        'variables': {
          'android_ndk_root_': '<(android_ndk_root)',
          'android_sysroot': '<(android_ndk_root)/platforms/android-<(android_target_platform)/arch-<(android_target_arch)',
          'android_llvmcxx': '<(android_ndk_root)/sources/cxx-stl/llvm-libc++',
        },
        'android_include': '<(android_sysroot)/usr/include',
        'conditions': [
          ['target_arch=="x64"', {
            'android_lib': '<(android_sysroot)/usr/lib64',
          }, {
            'android_lib': '<(android_sysroot)/usr/lib',
          }],
        ],
        'android_llvmcxx_include': '<(android_llvmcxx)/libcxx/include',
        'android_llvmcxx_libs': '<(android_llvmcxx)/libs',
      }],
    ],
    # Enable to use the system llvmcxx, otherwise statically
    # link the NDK one?
    'use_system_llvmcxx%': '<(android_webview_build)',
    'android_llvmcxx_library': 'c++_static',
  },  # variables
  'target_defaults': {
    'defines': [
      'ANDROID',
      'V8_ANDROID_LOG_STDOUT',
    ],
    'configurations': {
      'Release': {
        'cflags': [
          '-fomit-frame-pointer',
        ],
      },  # Release
    },  # configurations
    'cflags': [ '-std=gnu++11', '-Wno-abi', '-Wall', '-W', '-Wno-unused-parameter', '-Wno-extern-c-compat'],
    'cflags_cc': [ '-Wnon-virtual-dtor', '-fno-rtti', '-fno-exceptions',
                   # Note: Using -std=c++0x will define __STRICT_ANSI__, which
                   # in turn will leave out some template stuff for 'long
                   # long'.  What we want is -std=c++11, but this is not
                   # supported by GCC 4.6 or Xcode 4.2
                   '-std=gnu++11' ],
    'target_conditions': [
      ['_toolset=="target"', {
        'cflags!': [
          '-pthread',  # Not supported by Android toolchain.
          '-pedantic', # Warning galore
        ],
        'cflags': [
          '--sysroot=<(android_sysroot)',
          '-no-canonical-prefixes',
          '-ffunction-sections',
          '-funwind-tables',
          '-fstack-protector',
          '-fno-short-enums',
          '-Wa,--noexecstack',
          '-I<(android_llvmcxx_include)',
          '-I<(android_ndk_root)/sources/cxx-stl/gabi++/include',
          '-I<(android_ndk_root)/sources/android/support/include',
          '-I<(android_include)',
        ],
        'cflags_cc': [
          '-Wno-error=non-virtual-dtor',  # TODO(michaelbai): Fix warnings.
        ],
        'defines': [
          'ANDROID',
          #'__GNU_SOURCE=1',  # Necessary for clone()
          'HAVE_OFF64_T',
          'HAVE_SYS_UIO_H',
          'ANDROID_BINSIZE_HACK', # Enable temporary hacks to reduce binsize.
        ],
        'ldflags!': [
          '-pthread',  # Not supported by Android toolchain.
        ],
        'ldflags': [
          '--sysroot=<(android_sysroot)',
          '-no-canonical-prefixes',
          '-nostdlib',
          '-Wl,--no-undefined',
        ],
        'libraries!': [
            '-lrt',  # librt is built into Bionic.
            # Not supported by Android toolchain.
            # Where do these come from?  Can't find references in
            # any Chromium gyp or gypi file.  Maybe they come from
            # gyp itself?
            '-lpthread', '-lnss3', '-lnssutil3', '-lsmime3', '-lplds4', '-lplc4', '-lnspr4',
          ],
          'libraries': [
            '-l<(android_llvmcxx_library)',
            # Manually link the libgcc.a that the cross compiler uses.
            '<!($CC -print-libgcc-file-name)',
            '-lc',
            '-ldl',
            '-lm',
        ],
        'conditions': [
          ['android_webview_build==0', {
            'ldflags': [
              '-Wl,-rpath-link=<(android_lib)',
              '-L<(android_lib)',
            ],
          }],
          ['target_arch == "arm"', {
            'ldflags': [
              # Enable identical code folding to reduce size.
              '-Wl,--icf=safe',
            ],
          }],
          ['target_arch=="arm" and arm_version==7', {
            'cflags': [
              '-gcc-toolchain <(android_ndk_root)/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64',
              '-target armv7-none-linux-androideabi',
              '-march=armv7-a',
              '-mtune=cortex-a8',
              '-mfpu=vfpv3-d16',
              '-mfloat-abi=softfp',
              '-mthumb'
            ],
            'ldflags': [
              '-gcc-toolchain <(android_ndk_root)/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64',
              '-target armv7-none-linux-androideabi',
            ],
          }],
          # NOTE: The llvmcxx header include paths below are specified in
          # cflags rather than include_dirs because they need to come
          # after include_dirs. Think of them like system headers, but
          # don't use '-isystem' because the arm-linux-androideabi-4.4.3
          # toolchain (circa Gingerbread) will exhibit strange errors.
          # The include ordering here is important; change with caution.
          ['use_system_llvmcxx==0', {
            'conditions': [
              ['target_arch=="arm" and arm_version==7', {
                'ldflags': [
                  '-L<(android_llvmcxx_libs)/armeabi-v7a',
                ],
              }],
              ['target_arch=="arm" and arm_version < 7', {
                'ldflags': [
                  '-L<(android_llvmcxx_libs)/armeabi',
                ],
              }],
              ['target_arch=="mipsel"', {
                'ldflags': [
                  '-L<(android_llvmcxx_libs)/mips',
                ],
              }],
              ['target_arch=="ia32" or target_arch=="x87"', {
                'ldflags': [
                  '-L<(android_llvmcxx_libs)/x86',
                ],
              }],
              ['target_arch=="x64"', {
                'ldflags': [
                  '-L<(android_llvmcxx_libs)/x86_64',
                ],
              }],
              ['target_arch=="arm64"', {
                'ldflags': [
                  '-L<(android_llvmcxx_libs)/arm64-v8a',
                ],
              }],
            ],
          }],
          ['target_arch=="ia32" or target_arch=="x87"', {
            # The x86 toolchain currently has problems with stack-protector.
            'cflags!': [
              '-fstack-protector',
            ],
            'cflags': [
              '-fno-stack-protector',
            ],
          }],
          ['target_arch=="mipsel"', {
            # The mips toolchain currently has problems with stack-protector.
            'cflags!': [
              '-fstack-protector',
              '-U__linux__'
            ],
            'cflags': [
              '-fno-stack-protector',
            ],
          }],
          ['(target_arch=="arm" or target_arch=="arm64" or target_arch=="x64") and component!="shared_library"', {
            'cflags': [
              '-fPIE',
            ],
            'ldflags': [
              '-pie',
            ],
          }],
        ],
        'target_conditions': [
          ['_type=="executable"', {
            'conditions': [
              ['target_arch=="arm64" or target_arch=="x64"', {
                'ldflags': [
                  '-Wl,-dynamic-linker,/system/bin/linker64',
                ],
              }, {
                'ldflags': [
                  '-Wl,-dynamic-linker,/system/bin/linker',
                ],
              }]
            ],
            'ldflags': [
              '-Bdynamic',
              '-Wl,-z,nocopyreloc',
              # crtbegin_dynamic.o should be the last item in ldflags.
              '<(android_lib)/crtbegin_dynamic.o',
            ],
            'libraries': [
              # crtend_android.o needs to be the last item in libraries.
              # Do not add any libraries after this!
              '<(android_lib)/crtend_android.o',
            ],
          }],
          ['_type=="shared_library"', {
            'ldflags': [
              '-Wl,-shared,-Bsymbolic',
              '<(android_lib)/crtbegin_so.o',
            ],
          }],
          ['_type=="static_library"', {
            'ldflags': [
              # Don't export symbols from statically linked libraries.
              '-Wl,--exclude-libs=ALL',
            ],
          }],
        ],
      }],  # _toolset=="target"
      # Settings for building host targets using the system toolchain.
      ['_toolset=="host" and host_os!="mac"', {
        'cflags': [ '-pthread' ],
        'ldflags': [ '-pthread' ],
        'ldflags!': [
          '-Wl,-z,noexecstack',
          '-Wl,--gc-sections',
          '-Wl,-O1',
          '-Wl,--as-needed',
        ],
      }],
    ],  # target_conditions
  },  # target_defaults
}
