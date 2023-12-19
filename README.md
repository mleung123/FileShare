FileShare: A simple file system.

This system consists of two types of components: a central server on which files will be stored, and users, who can use the main server to store their data. 
The system can support many users, and can allow anyone on the Grinnell MATHLAN network to connect and share files.
The system can store any type of file, although certain functions might not work on files of specific types.

Startup:
To compile the code, run "make user" and "make central" in the console. 

To create an instance of the server, enter the following command:
./central [directorypath]

Files will be stored on the directorypath specified.

Users must connect to a server on program startup. To create a user instance, enter the following command:
./user [servername] [port number]


Features/User Commands:


Upload: 

Users can upload a file to the central server. Syntax:

upload [filename]

Download: 

Users can downlaod a file from the central server, to a directory of their choice. Syntax:

download [filename] [directory path]

Delete: 

Users can delete a specified file. Syntax: 

delete [filename]

Create:

Users can create a new file on the server. Syntax:
create [filename]

Display_all:

Displays all file names in the server. Syntax:

display_all

help:

prints out a list of commands for convenience

Checkout:

Checks out a specified file, downloading it to a specified directory.
While a file is checked out, it cannot be checked out by other users until it  is returned.

Return:

Returns a specified file, causing it to be reuploaded. Files can only be returned by the user that checked them out. Syntax:
return <filename>

Syntax:

checkout <filename> <targetDirectory>

Print:

Prints the contents of a given file, allowing users to view it without needing to download it.
Does not work on non-text files.

Status:

Prints the status(whether or not a file is checked out) of a given file. Syntax:

status <filename>




Example:

1. (terminal A) ./central /home/leungpat/csc213/fileShare/storage

Directory found. Files will be stored in /home/leungpat/csc213/fileShare/storage
Port Number: 41117
Server active. Type exit to quit.


2. (terminal B) ./user localhost 12345

Type in 'help' for a list of commands. Type exit to quit
Welcome, your id is: 4


3. (terminal C) ./user localhost 12345

Type in 'help' for a list of commands. Type exit to quit
Welcome, your id is: 5

4. (terminal B) display_all

asdf_lock.txt
helloworld_lock.txt
asdf.txt
helloworld.txt

5. (terminal B) status asdf.txt

This file is not checked out.

6. (terminal C) print asdf.txt

    (terminal C output)
        Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse iaculis, lectus id gravida placerat, elit nisl volutpat ante, non auctor augue felis eu ipsum. Nam lacinia vulputate arcu, sed iaculis ante ornare sit amet. Duis sed nunc arcu. Aenean semper sem sed urna vehicula, in scelerisque nunc faucibus. Nullam nec venenatis justo. Proin non ex elit. Maecenas vitae dolor pretium, facilisis ex vitae, pellentesque mi. Etiam suscipit turpis felis, sed commodo quam tristique commodo. Sed id molestie purus. Nulla eros dolor, rutrum eu iaculis eget, efficitur non augue. Suspendisse ornare volutpat urna eu tempus. 


7. (terminal C) upload small.png

    Terminal A output:
        Received file: /home/leungpat/csc213/fileShare/storage/small.png
        Lock file created successfully: /home/leungpat/csc213/fileShare/storage/small_lock.txt

    Terminal C output:
        Received file successfully

8. (terminal B) checkout asdf.txt /home/leungpat/csc213/fileShare/

    Terminal A output:
        file has been checked out by user 4
        sending finished
        This file is not checked out.
    
    Terminal B output:
        checkout asdf.txt /home/leungpat/csc213/fileShare/
        file has been checked out
        Received file: /home/leungpat/csc213/fileShare//asdf.txt

9. (terminal C) checkout asdf.txt /home/leungpat/csc213/fileShare/

    
    Terminal C output:
        File is checked out right now.

10. User 4 Modifies their copy of asdf.txt, replacing it with CSC-213 has been a great class!

11.  (terminal B) return asdf.txt 
    Terminal B output:
        File successfully returned.

12.  (terminal C) print asdf.txt
    Terminal C output: 
        CSC-213 has been a great class!
