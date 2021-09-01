##############################################################################
#                                                                            #
# miniproxy : mini proxy server                                              #
#                                                                            #
#  Copyright (C) 2021 Kuzmin Yaroslav <kuzmin.yaroslav@gmail.com>            #
#                                                                            #
# miniproxy is free software: you can redistribute it and/or modify it       #
# under the terms of the GNU General Public License as published by the      #
# Free Software Foundation, either version 3 of the License, or              #
# (at your option) any later version.                                        #
#                                                                            #
# miniproxy is distributed in the hope that it will be useful, but           #
# WITHOUT ANY WARRANTY; without even the implied warranty of                 #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                       #
# See the GNU General Public License for more details.                       #
#                                                                            #
# You should have received a copy of the GNU General Public License along    #
# with this program.  If not, see <http://www.gnu.org/licenses/>.            #
#                                                                            #
##############################################################################

project('miniproxy', 'c', version:'0.1', meson_version:'>=0.57.0'
       ,default_options:['warning_level=1', 'buildtype=debug', 'b_sanitize=address,undefined', 'c_std=c99'])

add_global_arguments('-D_XOPEN_SOURCE=700', language:'c')

source = ['src/miniproxy.c', 'src/thread_poll.c']

str_version = meson.project_version()
arr_version = str_version.split('.')
major_version = arr_version[0]
minor_version = arr_version[1]

message('Project name ' + meson.project_name() + '-' + str_version)

add_project_arguments('-DVERSION_MAJOR='+major_version, language:'c')
add_project_arguments('-DVERSION_MINOR='+minor_version, language:'c')
add_project_arguments('-D_XOPEN_SOURCE=700', language:'c')

thread_dep = dependency('threads')

binary = executable('miniproxy', source, dependencies : thread_dep)

soure_dir = include_directories('src')
source_test_thread = ['src/thread_poll.c', 'test/thread.c']
test_thread = executable('test_thread', source_test_thread, dependencies : thread_dep, include_directories : soure_dir)

test('thread', test_thread)

