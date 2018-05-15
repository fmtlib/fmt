param (
  [Parameter(Mandatory=$true)][string]$configuration,
  [Parameter(Mandatory=$true)][string]$jobid
)

$XSLInputElement = New-Object System.Xml.Xsl.XslCompiledTransform
$XslInputElement.Load("https://raw.githubusercontent.com/rpavlik/jenkins-ctest-plugin/master/ctest-to-junit.xsl")
$file = $(ls Testing\*\Test.xml) | Select -first 1
$XSLInputElement.Transform((Resolve-Path $file), (Join-Path (Resolve-Path .) "ctest-to-junit-results.xml"))
$wc = New-Object 'System.Net.WebClient'
$wc.UploadFile("https://ci.appveyor.com/api/testresults/junit/$jobid", (Resolve-Path .\ctest-to-junit-results.xml))
