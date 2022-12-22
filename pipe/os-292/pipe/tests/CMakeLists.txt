add_test(
  NAME EmptyTest
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/empty_test.py --executable $<TARGET_FILE:pipe>
)

function(add_exit_test name test)
  add_test(
    NAME ${name}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/exit_test.py --executable $<TARGET_FILE:pipe> --test ${test}
  )
endfunction()

add_exit_test(ExitFalseTest f)
add_exit_test(ExitInvalidTest i)
add_exit_test(ExitFalseInvalidTest fi)
add_exit_test(ExitInvalidFalseTest if)

function(add_child_test name children)
  add_test(
    NAME ${name}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/children_test.py --executable $<TARGET_FILE:pipe> --children ${children}
  )
endfunction()

add_child_test(ChildrenOneTest 1)
add_child_test(ChildrenTwoTest 2)
add_child_test(ChildrenThreeTest 3)
add_child_test(ChildrenFourTest 4)
add_child_test(ChildrenFiveTest 5)
add_child_test(ChildrenSixTest 6)
add_child_test(ChildrenSevenTest 7)
add_child_test(ChildrenEightTest 8)
add_child_test(ChildrenNineTest 9)
add_child_test(ChildrenTenTest 10)

function(add_pipe_test name pipes)
  add_test(
    NAME ${name}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/pipe_test.py --executable $<TARGET_FILE:pipe> --pipes ${pipes}
  )
endfunction()

add_pipe_test(PipeOneTest 1)
add_pipe_test(PipeTwoTest 2)
add_pipe_test(PipeThreeTest 3)
add_pipe_test(PipeFourTest 4)
add_pipe_test(PipeFiveTest 5)
add_pipe_test(PipeSixTest 6)
add_pipe_test(PipeSevenTest 7)
add_pipe_test(PipeEightTest 8)
add_pipe_test(PipeNineTest 9)

add_test(
  NAME StdinTest
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/stdin_test.py --executable $<TARGET_FILE:pipe>
)
