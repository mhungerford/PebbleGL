
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
                        '-mthumb',
                        '-ffunction-sections',
                        '-fdata-sections',
                        '-funroll-loops',
                        '-g',
                        '-Os',
                        '-Wall',
                        '-Wextra',
                        #'-Werror',
                        '-Wno-unused-parameter',
                        '-Wno-error=unused-function',
                        '-Wno-error=unused-variable' ]

    #Needed for math-sll.h asm functions
    ctx.env.CFLAGS.append('-Wa,-mimplicit-it=always')
    ctx.env.CFLAGS.append('-DFRAMEBUFFER_WIDTH=144')
    ctx.env.CFLAGS.append('-DFRAMEBUFFER_HEIGHT=144')

    ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'),
                    target='pebble-app.elf')

    ctx.pbl_bundle(elf='pebble-app.elf',
                   js=ctx.path.ant_glob('src/js/**/*.js'))
