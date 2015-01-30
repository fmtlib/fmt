add_custom_target(doc
  COMMAND doxygen
  COMMAND rm -rf html
  COMMAND sphinx-build -b html . html)
