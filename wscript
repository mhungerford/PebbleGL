
#
# This file is the default set of rules to compile a Pebble project.
#
# Feel free to customize this to your needs.
#

top = '.'
out = 'build'

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

def build(ctx):
    ctx.load('pebble_sdk')

    ctx.env.CFLAGS=['-std=c99',
                        '-mcpu=cortex-m3',
                        '-march=armv7-m',
                        '-mthumb',
                        '-msoft-float',
                        '-mfloat-abi=soft',
                        '-ffunction-sections',
                        '-fdata-sections',
                        '-fmessage-length=0',
                        '-fno-strict-aliasing',
                        '-fomit-frame-pointer',
                        '-ffast-math',
                        '-funroll-loops',
                       #'-g',
                        '-Os',
                        '-Wall',
                        '-Wextra',
                       #'-Werror',
                        '-Wl,--gc-sections',
                        '-Wno-unused-parameter',
                        '-Wno-error=unused-function',
                        '-Wno-error=unused-variable' ]

    #Needed for math-sll.h asm functions
    ctx.env.CFLAGS.append('-Wa,-mimplicit-it=always')
    ctx.env.CFLAGS.append('-DFRAMEBUFFER_WIDTH=144')
    ctx.env.CFLAGS.append('-DFRAMEBUFFER_HEIGHT=168')

    #ctx.env.CFLAGS.append('-falign-functions=1')
    #ctx.env.CFLAGS.append('-falign-jumps=1')
    #ctx.env.CFLAGS.append('-falign-loops=1')
    #ctx.env.CFLAGS.append('-falign-labels=1')

    #lto support
    #ctx.env.CFLAGS.append('-fuse-linker-plugin')
    #ctx.env.CFLAGS.append('-flto')
    #ctx.env.CFLAGS.append('-flto-partition=1to1')
    #ctx.env.CFLAGS.append('-ffat-lto-objects')
    #ctx.env.LINKFLAGS.append('-fuse-linker-plugin')
    #ctx.env.LINKFLAGS.append('-flto')
    #ctx.env.LINKFLAGS.append('-fwhole-program')
    #ctx.env.LINKFLAGS.append('-Os')
    #ctx.env.LINKFLAGS.append('-fno-delete-null-pointer-checks')
    #ctx.env.LINKFLAGS.append('-funroll-loops')
    #ctx.env.LINKFLAGS.append('-ffunction-sections')
    #ctx.env.LINKFLAGS.append('-fdata-sections')

    #ctx.env.CFLAGS.append('--specs=nano.specs')
    ctx.env.CFLAGS.append('-nostartfiles')
    #ctx.env.LINKFLAGS.append('--specs=nano.specs')
    ctx.env.LINKFLAGS.append('-nostartfiles')
    #ctx.env.LINKFLAGS.append('-Wl,--gc-sections')

    #ctx.env.CFLAGS.append('-flto')
    #ctx.env.CFLAGS.append('-fuse-linker-plugin')
    #ctx.env.LINKFLAGS=['-mcpu=cortex-m3','-mthumb','-Wl,--gc-sections','-Wl,--warn-common','-specs=nano.specs','-flto','-fuse-linker-plugin','-Os']
    #ctx.env.LINKFLAGS.append('-Wa,-mimplicit-it=thumb')

    ctx.pbl_program(source=['src/main.c','src/model_stl.c','src/miniGL/draw2d.c','src/miniGL/llvm_extendsfdf2.c','src/miniGL/miniGL.c'],
                    target='pebble-app.elf')

    ctx.pbl_bundle(elf='pebble-app.elf',
                   js=ctx.path.ant_glob('src/js/**/*.js'))
