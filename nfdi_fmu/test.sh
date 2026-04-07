url_fmu_start="http://localhost:8080/api/v2/gateway/fmu/FMU_server/FMI_service/FMU_Start"

curl $url_fmu_start -s -o /dev/null -X PUT
