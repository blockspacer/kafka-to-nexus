from helpers.kafkahelpers import create_producer, send_writer_command
from time import sleep
from subprocess import check_output

# Simple system test for checking that a file writer only responds to commands given with the correct service-id.
# Exit command tested, shouldn't matter if the file-writing has started or not.

def test_ignores_commands_with_incorrect_id(docker_compose_multiple_instances):
    sleep(10)
    producer = create_producer()
    send_writer_command("commands/writer-exit-single.json", producer, "TEST_writerCommandMultiple")
    sleep(10)
    containers = check_output('docker ps', shell=True)
    assert b"filewriter1" in containers
    assert b"filewriter2" not in containers
