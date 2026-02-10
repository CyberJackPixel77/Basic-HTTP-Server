$server = "127.0.0.1"
$port = 8080

function Send-HttpRequest {
    param (
        [string]$testName,
        [string]$request
    )
    
    Write-Host "`n[Test]: $testName" -ForegroundColor Cyan
    try {
        $client = New-Object System.Net.Sockets.TcpClient($server, $port)
        $stream = $client.GetStream()
        $writer = New-Object System.IO.StreamWriter($stream)
        $writer.AutoFlush = $true

        $writer.Write($request)
        
        # Читаем ответ
        $reader = New-Object System.IO.StreamReader($stream)
        $response = $reader.ReadToEnd()
        while ($stream.DataAvailable -or !$reader.EndOfStream) {
            $line = $reader.ReadLine()
            if ($null -eq $line) { break }
            $response += $line + "`n"
            if ($line -eq "") { break } # Конец заголовков
        }
        
        Write-Host $response -ForegroundColor Gray
        $client.Close()
    }
    catch {
        Write-Host "Connection Err: $($_.Exception.Message)" -ForegroundColor Red
    }
}

Write-Host "Stating server test`n-------------- OPTIONS -------------`n"

# 1. OPTIONS METHOD TEST
Send-HttpRequest "Base OPTIONS * (HTTP/1.0)" `
"OPTIONS * HTTP/1.0`r`n`r`n"

Send-HttpRequest "Base OPTIONS *" `
"OPTIONS * HTTP/1.1`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "OPTIONS * HTTP2" `
"OPTIONS * HTTP/2`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "OPTIONS * HTTP3" `
"OPTIONS * HTTP/3`r`nHost: My-site`r`n`r`n"

#=======================================================
Write-Host "Testing strange behavior...`n"

Send-HttpRequest "Incorrect path, linux way path" `
"OPTIONS /SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "Incorrect path 2, doubleslash" `
"OPTIONS SERVER\\\\File1.txt HTTP/1.1`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "Incorrect path 3, null symbol" `
"OPTIONS SERVER\File1.txt%00 HTTP/1.1`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "Incorrect path 4, no SERVER key" `
"OPTIONS \Image.jpeg HTTP/1.1`r`nHost: My-site`r`n`r`n"
Write-Host "---------------------------------------------"
#=======================================================
Write-Host "Testing file options...`n"
Send-HttpRequest "Incorrect proto to existing file" `
"OPTIONS SERVER\random.txt HTTP/3`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "HTTP/1.1 Without hosts field" `
"OPTIONS SERVER\random.txt HTTP/1.1`r`n`r`n"

Send-HttpRequest "Non-existing file" `
"OPTIONS SERVER\missing.txt HTTP/1.1`r`nHost: My-site`r`n`r`n"

Write-Host "=============== CHECKING FILE ACCESS RULES ===============`nChecking 'File.ini'..."

Send-HttpRequest "OPTIONS file.ini" `
"OPTIONS SERVER\File.ini HTTP/1.1`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "GET file.ini" `
"GET SERVER\File.ini HTTP/1.1`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "HEAD file.ini" `
"HEAD SERVER\File.ini HTTP/1.1`r`nHost: My-site`r`n`r`n"

#----------------------------------------------------------------------------
Write-Host "---------------------------------------------`nChecking 'Chrome.txt'...`nTriggering filtering with incorrect UA..."
Send-HttpRequest "GET, User-Agent: Mozilla, HTTP/1.1" `
"GET SERVER\Chrome.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Mozilla`r`n`r`n"

Send-HttpRequest "HEAD, User-Agent: Mozilla, HTTP/1.1" `
"HEAD SERVER\Chrome.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Mozilla`r`n`r`n"

Send-HttpRequest "OPTIONS, User-Agent: Mozilla, HTTP/1.1" `
"OPTIONS SERVER\Chrome.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Mozilla`r`n`r`n"

Send-HttpRequest "Incorrect proto, User-Agent: Mozilla, HTTP/3" `
"HEAD SERVER\Chrome.txt HTTP/3`r`nHost: My-site`r`nUser-Agent: Mozilla`r`n`r`n"

#------
Write-Host "---------------------------------------------`n"
Write-Host "Trying normal UA...`n"
Send-HttpRequest "GET, User-Agent: Google, HTTP/1.1" `
"GET SERVER\Chrome.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Google`r`n`r`n"

Send-HttpRequest "HEAD, User-Agent: Google, HTTP/1.1" `
"HEAD SERVER\Chrome.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Google`r`n`r`n"

Send-HttpRequest "OPTIONS, User-Agent: Google, HTTP/1.1" `
"OPTIONS SERVER\Chrome.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Google`r`n`r`n"

Send-HttpRequest "Incorrect proto, User-Agent: Google, HTTP/3" `
"HEAD SERVER\Chrome.txt HTTP/3`r`nHost: My-site`r`nUser-Agent: Google`r`n`r`n"
#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------
Write-Host "---------------------------------------------"
Write-Host "Checking 'Firefox.txt'...`nTriggering filtering with incorrect UA..."
Send-HttpRequest "GET, User-Agent: Google, HTTP/1.1" `
"GET SERVER\Firefox.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Google`r`n`r`n"

Send-HttpRequest "HEAD, User-Agent: Google, HTTP/1.1" `
"HEAD SERVER\Firefox.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Google`r`n`r`n"

Send-HttpRequest "OPTIONS, User-Agent: Google, HTTP/1.1" `
"OPTIONS SERVER\Firefox.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Google`r`n`r`n"

Send-HttpRequest "Incorrect proto, User-Agent: Google, HTTP/3" `
"HEAD SERVER\Firefox.txt HTTP/3`r`nHost: My-site`r`nUser-Agent: Google`r`n`r`n"
#----
Write-Host "---------------------------------------------`n"
Write-Host "Trying normal UA...`n"
Send-HttpRequest "GET, User-Agent: Mozilla, HTTP/1.1" `
"GET SERVER\Firefox.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Mozilla`r`n`r`n"

Send-HttpRequest "HEAD, User-Agent: Mozilla, HTTP/1.1" `
"HEAD SERVER\Firefox.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Mozilla`r`n`r`n"

Send-HttpRequest "OPTIONS, User-Agent: Mozilla, HTTP/1.1" `
"OPTIONS SERVER\Firefox.txt HTTP/1.1`r`nHost: My-site`r`nUser-Agent: Mozilla`r`n`r`n"

Send-HttpRequest "Incorrect proto, User-Agent: Mozilla, HTTP/3" `
"HEAD SERVER\Firefox.txt HTTP/3`r`nHost: My-site`r`nUser-Agent: Mozilla`r`n`r`n"
#-------------------------------------------------------------------------------
Write-Host "---------------------------------------------`nChecking 'File1.txt'...`nTriggering filtering with 'Accept-Language: en-US'..."
Send-HttpRequest "GET, Accept-Language: en-US, HTTP/1.1" `
"GET SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`nAccept-Language: en-US`r`n`r`n"

Send-HttpRequest "HEAD, Accept-Language: en-US, HTTP/1.1" `
"HEAD SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`nAccept-Language: en-US`r`n`r`n"

Send-HttpRequest "OPTIONS, Accept-Language: en-US, HTTP/1.1" `
"OPTIONS SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`nAccept-Language: en-US`r`n`r`n"

Send-HttpRequest "Incorrect proto, Accept-Language: en-US, HTTP/3" `
"HEAD SERVER\File1.txt HTTP/3`r`nHost: My-site`r`nAccept-Language: en-US`r`n`r`n"
#--------
Write-Host "---------------------------------------------`nTrying without 'Accept-Language' at all...`n"
Send-HttpRequest "GET, HTTP/1.1" `
"GET SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "HEAD, HTTP/1.1" `
"HEAD SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "OPTIONS, HTTP/1.1" `
"OPTIONS SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`n`r`n"

Send-HttpRequest "Incorrect proto" `
"HEAD SERVER\Firefox.txt HTTP/3`r`nHost: My-site`r`nUser-Agent: Google`r`n`r`n"
#--------
Write-Host "---------------------------------------------`nTrying with ru-Ru language...`n"
Send-HttpRequest "GET, Accept-Language: ru-RU, HTTP/1.1" `
"GET SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "HEAD, Accept-Language: ru-RU, HTTP/1.1" `
"HEAD SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "OPTIONS, Accept-Language: ru-RU, HTTP/1.1" `
"OPTIONS SERVER\File1.txt HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "Incorrect proto, Accept-Language: ru-RU, HTTP/3" `
"HEAD SERVER\File1.txt HTTP/3`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"
#===============================================================================
Write-Host "---------------------------------------------`nChecking other files`nImage.jpeg`n"
Send-HttpRequest "GET, Image.jpeg, HTTP/1.1" `
"GET SERVER\Image.jpeg HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "HEAD, Image.jpeg, HTTP/1.1" `
"HEAD SERVER\Image.jpeg HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "OPTIONS, Image.jpeg, HTTP/1.1" `
"OPTIONS SERVER\Image.jpeg HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "Incorrect proto, Image.jpeg: ru-RU, HTTP/3" `
"HEAD SERVER\Image.jpeg HTTP/3`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"
#-
Write-Host "Checking 'hosts' file...`n"

Send-HttpRequest "GET, hosts, HTTP/1.1" `
"GET SERVER\hosts HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "HEAD, hosts, HTTP/1.1" `
"HEAD SERVER\hosts HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "OPTIONS, hosts, HTTP/1.1" `
"OPTIONS SERVER\hosts HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "Incorrect proto, hosts: ru-RU, HTTP/3" `
"HEAD SERVER\hosts HTTP/3`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"
#=================================================================================
Send-HttpRequest "GET, site.css, HTTP/1.1" `
"GET SERVER\site.css HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "HEAD, site.css, HTTP/1.1" `
"HEAD SERVER\site.css HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "OPTIONS, site.css, HTTP/1.1" `
"OPTIONS SERVER\site.css HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "Incorrect proto, site.css, HTTP/3" `
"HEAD SERVER\site.css HTTP/3`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"
#===========================
Send-HttpRequest "GET, site.html, HTTP/1.1" `
"GET SERVER\site.html HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "HEAD, site.html, HTTP/1.1" `
"HEAD SERVER\site.html HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "OPTIONS, site.html, HTTP/1.1" `
"OPTIONS SERVER\site.html HTTP/1.1`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"

Send-HttpRequest "Incorrect proto, site.html, HTTP/3" `
"HEAD SERVER\site.html HTTP/3`r`nHost: My-site`r`nAccept-Language: ru-RU`r`n`r`n"
Write-Host "=============================================="
Write-Host "`n[All test copleted]" -ForegroundColor Green
Read-Host -Prompt "Press Enter to continue"