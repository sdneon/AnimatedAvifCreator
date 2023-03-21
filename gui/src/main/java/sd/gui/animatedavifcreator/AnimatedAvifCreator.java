package sd.gui.animatedavifcreator;

import javax.imageio.ImageIO;
//import javax.imageio.stream.FileImageOutputStream;
//import javax.imageio.stream.ImageOutputStream;
import java.io.*;
import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import components.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Vector;
import sd.gui.libAvifHeicJNI;

/**
 *
 * @author Neon
 */
public class AnimatedAvifCreator extends JFrame implements ActionListener {
	private static final long serialVersionUID = 6968009390810156032L;
	protected JFileChooser imageChooser = null;
	protected ImageFilter imageFilter = null;
	protected JTextField txtDelay, txtOutputFilename;
	protected JCheckBox cbLoop, cbAddReverse;
	protected JButton butChooseImages, butCreate, butChooseOutput,
		butResetDelays;
	protected JPanel paneMid;
	protected TabPanel listImages;
	protected DefaultListModel<ImageIcon> listModel = new DefaultListModel<ImageIcon>();
	protected File images[];
	protected String outputFilePath = "example.avif";

	public AnimatedAvifCreator()
	{
		super("Animated AVIF Creator");
		this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		initComponents();
	}
	
	protected void initComponents()
	{
		JPanel paneTop = new JPanel();
		paneTop.setLayout(new FlowLayout());
		butChooseImages = new JButton("(1) Choose images");
		butChooseImages.addActionListener(this);
		butChooseImages.setMnemonic('i');
		paneTop.add(butChooseImages);
		JLabel labDelay = new JLabel("Delay (ms):");
		labDelay.setDisplayedMnemonic('d');
		paneTop.add(labDelay);
		txtDelay = new JTextField("250");
		txtDelay.setFocusAccelerator('d');
		paneTop.add(txtDelay);
		butResetDelays = new JButton("Reset delays");
		butResetDelays.addActionListener(this);
		paneTop.add(butResetDelays);

		cbLoop = new JCheckBox("Loop", true);
		cbLoop.setMnemonic('l');
		cbAddReverse = new JCheckBox("Add reverse frames", true);
		cbAddReverse.setMnemonic('r');
		paneTop.add(cbLoop);
		paneTop.add(cbAddReverse);
		
		JPanel paneTop2 = new JPanel();
		paneTop2.setLayout(new BorderLayout());
		add(paneTop2, BorderLayout.NORTH);
		paneTop2.add(paneTop, BorderLayout.NORTH);

		listImages = new TabPanel(listModel);
		add(listImages, BorderLayout.WEST);

		paneMid = new JPanel();
		add(paneMid);
		
		JPanel paneBtm = new JPanel();
        paneBtm.setLayout(new BorderLayout());
		butChooseOutput = new JButton("(2) Choose output filename:");
		butChooseOutput.addActionListener(this);
		butChooseOutput.setMnemonic('o');
		paneBtm.add(butChooseOutput, BorderLayout.WEST);
		txtOutputFilename = new JTextField(outputFilePath);
		paneBtm.add(txtOutputFilename);
		butCreate = new JButton("(3) Create AVIF");
		butCreate.addActionListener(this);
		butCreate.setMnemonic('c');
		paneBtm.add(butCreate, BorderLayout.EAST);
		paneTop2.add(paneBtm, BorderLayout.SOUTH);
	}

	public static int safeParseInt(String s, int def)
	{
		try
		{
			return Integer.parseInt(s);
		}
		catch (NumberFormatException ex)
		{}
		return def;
	}
	
	protected int getDelay()
	{
		return safeParseInt(txtDelay.getText(), 250);
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		Object src = e.getSource();
		if (src == butCreate)
		{
			if (images == null) {
	        	System.out.println("No images selected.");
	        	e.setSource(butChooseImages);
	        	actionPerformed(e);
	        	return;
			}
			createAnimAvif(false);
		}
		else if (src == butChooseImages)
		{
			createFileChooserIfNeeded();
			imageChooser.setMultiSelectionEnabled(true);
			imageFilter.setGifOnly(false);

	        //Show it.
	        int returnVal = imageChooser.showDialog(null, "Choose images");

	        //Process the results.
	        if (returnVal != JFileChooser.APPROVE_OPTION) {
	        	System.out.println("Cancelled by user.");
	        	return;
	        }
	        images = imageChooser.getSelectedFiles();
	        populateList();
		}
		else if (src == butChooseOutput)
		{
			createFileChooserIfNeeded();
			imageChooser.setMultiSelectionEnabled(false);
			imageFilter.setGifOnly(true);

			//Show it.
	        int returnVal = imageChooser.showDialog(null, "Select output AVIF filename");

	        //Process the results.
	        if (returnVal != JFileChooser.APPROVE_OPTION) {
	        	System.out.println("Cancelled by user.");
	        	return;
	        }
	        outputFilePath = imageChooser.getSelectedFile().getAbsolutePath();
	        txtOutputFilename.setText(outputFilePath);
		}
		else if (src == butResetDelays)
		{
			String newDelay = "" + getDelay(); //also resets invalid, non-numeric text
			Enumeration<ImageIcon> content = listModel.elements();
			while (content.hasMoreElements())
			{
				ImageIcon icon = content.nextElement();
				icon.setDescription(newDelay);
			}
			listImages.updateTexts();
		}
	}

	protected void populateList()
	{
		if ((images == null) || (images.length <= 0))
		{
			return;
		}
		listModel.clear();
		String delayMs = txtDelay.getText();
        for (File image : images) {
        	listModel.add(listModel.size(), new ImageIcon(new ImageIcon(image.getAbsolutePath(), delayMs)
    			.getImage().getScaledInstance(100, 100, Image.SCALE_DEFAULT), delayMs));
        }
        listImages.setModel(listModel);
        pack();
	}

	protected void createAnimAvif(boolean override)
	{
		setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
        //ImageOutputStream output;
        BufferedOutputStream output;
		try {
			outputFilePath = txtOutputFilename.getText();
			File file = new File(outputFilePath);
			if (file.exists())
			{
				if (!override)
				{
					int input = JOptionPane.showConfirmDialog(null, "Override existing image?");
			        // 0=yes, 1=no, 2=cancel
					if (input != 0)
					{
						System.out.println("Cancelled by user.");
						return;
					}
				}
				file.delete();
			}
			//output = new FileImageOutputStream(file);
            output = new BufferedOutputStream(new FileOutputStream(file));
	        boolean addRev = cbAddReverse.isSelected();
            int numFrames = addRev? (images.length * 2): images.length;
            int frames[][] = new int[numFrames][];
            long frameIntervals[] = new long[numFrames];

            int w = -1, h = 0,
                i = 0, j = 0;
            for (File image : images) {
	            BufferedImage next = ImageIO.read(image);
	            if (next != null)
	            {
       	        	if (w < 0)
                    {
                        w = next.getWidth();
                        h = next.getHeight();
                    }
                    frames[j] = next.getRGB(0, 0, w, h, null, 0, w);
                    frameIntervals[j] = getDelayOfFrame(i);
                    if (addRev)
                    {
                        frames[numFrames - j - 1] = frames[j];
                        frameIntervals[numFrames - j - 1] = frameIntervals[j];
                    }
                    ++j; //actual available
	            }
                ++i;
	        }
            numFrames = j;
            
            if (numFrames > 0)
            {
                boolean ok = libAvifHeicJNI.AvifHeicWrite(output, frames, w, h, numFrames, 60, frameIntervals);
                if (!ok)
                {
                    System.out.println("Failed to save animated AVIF!");
                }
            }
            frames = null;

            output.flush();
	        output.close();
	        System.out.println("Animated AVIF created =)");
	        
	        //Show AVIF
	        ImageIcon icon = new ImageIcon(outputFilePath, ""); //createImageIcon(outputFilePath, "");
	        icon.getImage().flush();
	        JLabel label1 = new JLabel(icon, JLabel.CENTER);
	        paneMid.removeAll();
	        paneMid.add(label1);
	        pack();
		} catch (IOException e1) {
			e1.printStackTrace();
		}
		finally {
			setCursor(Cursor.getDefaultCursor());
		}
	}

	protected void createFileChooserIfNeeded()
	{
		if (imageChooser == null)
		{
			imageChooser = new JFileChooser();
		    //Add a custom file filter and disable the default
		    //(Accept All) file filter.
	        imageChooser.addChoosableFileFilter(imageFilter = new ImageFilter());
	        imageChooser.setAcceptAllFileFilterUsed(false);
	        imageChooser.setMultiSelectionEnabled(true);

		    //Add custom icons for file types.
	        imageChooser.setFileView(new ImageFileView());

		    //Add the preview pane.
	        imageChooser.setAccessory(new ImagePreview(imageChooser));
		}
	}

	protected int getDelayOfFrame(int j)
	{
		String s = ((ImageIcon)listModel.getElementAt(j)).getDescription();
		return safeParseInt(s, 250);
	}

	/** Returns an ImageIcon, or null if the path was invalid. */
//	protected ImageIcon createImageIcon(String path, String description)
//	{
//	    java.net.URL imgURL = getClass().getResource(path);
//	    if (imgURL != null) {
//	        ImageIcon icon =  new ImageIcon(imgURL, description);
//	        icon.getImage().flush();
//	        return icon;
//	    } else {
//	        System.err.println("Couldn't find file: " + path);
//	        return null;
//	    }
//	}

	
	class FrameEntry extends JPanel implements FocusListener {
		private static final long serialVersionUID = -5736079728301186202L;
		protected ImageIcon icon;
		protected JLabel label;
		protected JTextField txtDelay;
		public FrameEntry(ImageIcon icon) {
			setOpaque(true);
			label = new JLabel(icon);
			label.setHorizontalAlignment(JLabel.CENTER);
			label.setVerticalAlignment(JLabel.CENTER);
			label.setVerticalTextPosition(JLabel.BOTTOM);
			txtDelay = new JTextField(icon.getDescription());
			txtDelay.setEditable(true);
			txtDelay.setEnabled(true);
			setLayout(new BorderLayout());
			add(label);
			add(txtDelay, BorderLayout.SOUTH);
			addFocusListener(this);
			label.addFocusListener(this);
			txtDelay.addFocusListener(this);
			txtDelay.getDocument().addDocumentListener(new DocumentListener() {
				  public void changedUpdate(DocumentEvent e) {
				      updateDelaySelectedFrame();
				  }
				  public void removeUpdate(DocumentEvent e) {
				      updateDelaySelectedFrame();
				  }
				  public void insertUpdate(DocumentEvent e) {
				      updateDelaySelectedFrame();
				  }

				  public void updateDelaySelectedFrame() {
					  String newDelay = txtDelay.getText();
					  int value = safeParseInt(newDelay, 250);
				      if (value < 0)
				      {
					      JOptionPane.showMessageDialog(null,
					          "Error: Please enter number non-negative number", "Error Message",
					          JOptionPane.ERROR_MESSAGE);
				      }
			    	  icon.setDescription(newDelay);
				  }
			});
			
			setPreferredSize(new Dimension(100, 130));
		}

		public void focusGained(FocusEvent e) {
	        System.out.println("focusGained");
			txtDelay.requestFocus();
	    }

	    public void focusLost(FocusEvent e) {
	        System.out.println("focusGained");
	    }

		/*
		* This method finds the image and text corresponding
		* to the selected value and returns the label, set up
		* to display the text and image.
		*/
		public void updateUI(
		                    JList list,
		                    Object value,
		                    int index,
		                    boolean isSelected,
		                    boolean cellHasFocus)
		{
			//Get the selected index. (The index parameter isn't
			//always valid, so just use the value.)
			ImageIcon icon = (ImageIcon)value;
			if (isSelected) {
				setBackground(list.getSelectionBackground());
				setForeground(list.getSelectionForeground());
			} else {
				setBackground(list.getBackground());
				setForeground(list.getForeground());
			}
			
			//Set the icon and text.  If icon was null, say so.
			label.setIcon(icon);
			txtDelay.setText(icon.getDescription());
		}
	}

	class TabPanel extends JPanel {
		protected Vector<FrameEntry> vecFrames = new Vector<FrameEntry>(10); 
		protected DefaultListModel<ImageIcon> listModel;
		protected JPanel innerPanel;

		public TabPanel(DefaultListModel<ImageIcon> listModel)
		{
		    innerPanel = new JPanel();
		    innerPanel.setLayout(new BoxLayout(innerPanel, BoxLayout.Y_AXIS));

		    JScrollPane scrollPane = new JScrollPane(innerPanel);
		    //scrollPane.setPreferredSize(new Dimension(400, 200));
		    this.add(scrollPane);
		    
		    setModel(listModel);
		}
		
		public void setModel(DefaultListModel<ImageIcon> listModel)
		{
			this.listModel = listModel;
			innerPanel.removeAll();
			vecFrames.clear();
			
			Enumeration<ImageIcon> content = listModel.elements();
			while (content.hasMoreElements())
			{
				ImageIcon icon = content.nextElement();
				FrameEntry e = new FrameEntry(icon);
				innerPanel.add(e);
				vecFrames.add(e);
			}
		}
		
		public void updateTexts()
		{
			int i = 0;
			Enumeration<ImageIcon> content = listModel.elements();
			while (content.hasMoreElements())
			{
				ImageIcon icon = content.nextElement();
				FrameEntry e = vecFrames.elementAt(i);
				e.txtDelay.setText(icon.getDescription());
				++i;
			}
		}
	}
	
    public static void main(String[] args) throws Exception {
    	AnimatedAvifCreator g = new AnimatedAvifCreator();
    	g.pack();
    	g.setVisible(true);
    }
}
