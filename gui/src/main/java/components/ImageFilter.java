package components;

import java.io.File;
import javax.swing.*;
import javax.swing.filechooser.*;

/* ImageFilter.java is used by FileChooserDemo2.java. */
public class ImageFilter extends FileFilter {
	protected boolean gifOnly = false;
	public boolean isGifOnly() {
		return gifOnly;
	}
	public void setGifOnly(boolean gifOnly) {
		this.gifOnly = gifOnly;
	}
	public ImageFilter(boolean gifOnly) {
		super();
		this.gifOnly = gifOnly;
	}
	public ImageFilter() {
		super();
	}

    //Accept all directories and all gif, jpg, tiff, or png files.
    public boolean accept(File f) {
        if (f.isDirectory()) {
            return true;
        }

        String extension = Utils.getExtension(f);
        if (extension != null) {
        	if (gifOnly)
        	{
        		return extension.equals(Utils.gif);
        	}
            if (extension.equals(Utils.tiff) ||
                extension.equals(Utils.tif) ||
                extension.equals(Utils.gif) ||
                extension.equals(Utils.jpeg) ||
                extension.equals(Utils.jpg) ||
                extension.equals(Utils.png)) {
                    return true;
            } else {
                return false;
            }
        }

        return false;
    }

    //The description of this filter
    public String getDescription() {
        return "Just Images";
    }
}
