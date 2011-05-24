---------------------------------------
PANTHEON MIGRATE MODULE README.TXT FILE
---------------------------------------

Table of Contents:
1.) Migrating to Pantheon 
2.) Installing PEAR 
3.) Installing Archive_Tar
4.) Troubleshooting Errors
=======================================

1.) Migrating to Pantheon

In order to migrate your site to Pantheon, you need to install and enable both the Backup and Migrate Module (http://drupal.org/project/backup_migrate) and the Pantheon Migrate module. Afterwards, using a user with the appropriate permissions you can initiate a migration on your local Drupal site here (admin/content/backup_migrate/export/pantheon) by creating a Pantheon Archive file. To complete the migration you "send to pantheon" to Pantheon Archive file. 

2.) Installing PEAR 

The Pantheon Migration module requires the use of PEAR to load the Archive_Tar library which is necessary for the migration. Most systems already have PEAR installed, but if you need to install it you can do so by following these directions - http://pear.php.net/manual/en/installation.php.

3.) Installing Archive_Tar

The Pantheon Migration module requires the Archive_Tar PEAR extension to package your site for migration. You can download the Archive_Tar extension by following these instructions (http://pear.php.net/package/Archive_Tar/download) or by manually downloading the package (http://download.pear.php.net/package/Archive_Tar-1.3.7.tgz) and placing the Tar.php file in the pantheon_migrate folder. 

4.) Troubleshooting Errors

The process of migrating a Drupal site can be complex and there are a number of errors that you might run into during that process. Please consult these common problems and the contact Pantheon Support (https://getpantheon.com/support/contacting-pantheon-support) if you have further questions.

Memory Problems Creating a Pantheon Archive - The archive process often takes more memory than is normally available to PHP. If you need more memory, read this documentation (http://drupal.org/node/207036) on how to increase the amount of available memory.

Problems with Symbolic Links - If your site makes use of symbolic links, they have been known to confuse the Pantheon Archive functionality. If you see problems with symbolic links, we recommend you review your symbolic links before attempting to migrate again.

Problems with Firewalls or Localhost Migrations - The Pantheon Migration process requires that the Pantheon servers are able to access your Pantheon Archive file directly. If your site is behind a firewall or if your site is on localhost, the automatic migration will not work. Instead, download the archive file and upload it manually to Pantheon.

Pantheon Error: "Not a valid tar/zip archive" - There are a number of things that can cause this error, but the most common one is that Pantheon cannot read the file correctly. If you see this problem, try downloading the archive file and uplading it manually to Pantheon. 
