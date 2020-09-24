
$output_text = gh pr create --title "test" --body "test" --repo danewalton/azure-sdk-for-c

$endpoint = Where-Object {$output_text.Contains("https://github.com")}

Write-Host $endpoint
