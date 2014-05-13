add_custom_command(OUTPUT html/index.html
  COMMAND doxygen
  COMMAND rm -rf html
  COMMAND ../sphinx/sphinx-build.py -b html . html
  DEPENDS ../format.h Doxyfile conf.py index.rst _templates/layout.html)
add_custom_target(doc DEPENDS html/index.html)
